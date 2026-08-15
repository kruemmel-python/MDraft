#include "mtx/native_window_win32.hpp"
#include "mtx/layout_engine.hpp"
#include "mtx/file_io.hpp"
#include "mtx/render_gdi_win32.hpp"
#include "mtx/win32_clipboard_bridge.hpp"
#include "mtx/file_index.hpp"
#include "mtx/diagnostics.hpp"
#include "mtx/workspace_search.hpp"
#include "mtx/workspace_symbol_nav.hpp"
#include "mtx/image_suggestions.hpp"
#include "mtx/link_validation.hpp"
#include "mtx/workspace_lint.hpp"
#include "mtx/git_adapter.hpp"
#include "mtx/snippets.hpp"
#include <algorithm>
#include <cctype>
#include <commdlg.h>
#include <shlobj.h>
#include <cstdio>
#include <filesystem>
#include <stdexcept>

#ifndef MDRAFT_VERSION
#define MDRAFT_VERSION "0.0.0-dev"
#endif

namespace mtx {
namespace {
const wchar_t* kClassName = L"MDraftNativeWin32Window";
constexpr WORD kWinCursorArrow = 32512;
constexpr WORD kWinCursorIBeam = 32513;

HCURSOR load_system_cursor(WORD id) {
  return LoadCursorW(nullptr, MAKEINTRESOURCEW(id));
}

std::wstring widen(const std::string& s) {
  if (s.empty()) return {};
  int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
  if (n <= 0) {
    std::wstring out;
    out.reserve(s.size());
    for (unsigned char c : s) out.push_back(static_cast<wchar_t>(c));
    return out;
  }
  std::wstring out(static_cast<std::size_t>(n), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), out.data(), n);
  return out;
}

std::string narrow_utf8(const wchar_t* text) {
  if (text == nullptr || *text == L'\0') return {};
  const int n = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
  if (n <= 1) return {};
  std::string out(static_cast<std::size_t>(n - 1), '\0');
  WideCharToMultiByte(CP_UTF8, 0, text, -1, out.data(), n, nullptr, nullptr);
  return out;
}

bool is_utf8_continuation(unsigned char c) noexcept {
  return (c & 0xC0) == 0x80;
}

std::size_t utf8_codepoint_len(const std::string& text, std::size_t pos, std::size_t end) noexcept {
  if (pos >= end || pos >= text.size()) return 0;
  const unsigned char c = static_cast<unsigned char>(text[pos]);
  const std::size_t available = std::min(end, text.size()) - pos;
  if (c < 0x80) return 1;
  if (c >= 0xC2 && c <= 0xDF && available >= 2 &&
      is_utf8_continuation(static_cast<unsigned char>(text[pos + 1]))) {
    return 2;
  }
  if (c >= 0xE0 && c <= 0xEF && available >= 3 &&
      is_utf8_continuation(static_cast<unsigned char>(text[pos + 1])) &&
      is_utf8_continuation(static_cast<unsigned char>(text[pos + 2]))) {
    return 3;
  }
  if (c >= 0xF0 && c <= 0xF4 && available >= 4 &&
      is_utf8_continuation(static_cast<unsigned char>(text[pos + 1])) &&
      is_utf8_continuation(static_cast<unsigned char>(text[pos + 2])) &&
      is_utf8_continuation(static_cast<unsigned char>(text[pos + 3]))) {
    return 4;
  }
  return 1;
}

std::string extract_filename_tail(const std::string& p) {
  const std::size_t slash = p.find_last_of("/\\");
  return slash == std::string::npos ? p : p.substr(slash + 1);
}

std::string extract_directory_head(const std::string& p) {
  const std::size_t slash = p.find_last_of("/\\");
  return slash == std::string::npos ? std::string(".") : p.substr(0, slash);
}

std::string make_relative_asset_href(const std::string& current_file, const std::string& asset_path) {
  if (asset_path.empty()) return {};
  std::error_code ec;
  std::filesystem::path asset = std::filesystem::u8path(asset_path);
  if (current_file.empty()) return asset.filename().u8string();
  std::filesystem::path base = std::filesystem::u8path(extract_directory_head(current_file));
  std::filesystem::path rel = std::filesystem::relative(asset, base, ec);
  if (ec || rel.empty()) return asset.u8string();
  return rel.generic_u8string();
}

std::pair<std::size_t, std::size_t> word_bounds_for_snippet(const std::string& text, std::size_t cursor) {
  cursor = std::min(cursor, text.size());
  std::size_t b = cursor;
  std::size_t e = cursor;
  while (b > 0) {
    const unsigned char c = static_cast<unsigned char>(text[b - 1]);
    if (!std::isalnum(c) && c != '_' && c != '-') break;
    --b;
  }
  while (e < text.size()) {
    const unsigned char c = static_cast<unsigned char>(text[e]);
    if (!std::isalnum(c) && c != '_' && c != '-') break;
    ++e;
  }
  return {b, e};
}

std::string truncate_middle(const std::string& text, std::size_t max_chars) {
  if (text.size() <= max_chars) return text;
  if (max_chars < 12) return text.substr(0, max_chars);
  const std::size_t head = (max_chars - 3) / 2;
  const std::size_t tail = max_chars - 3 - head;
  return text.substr(0, head) + "..." + text.substr(text.size() - tail);
}

COLORREF style_fg(TextStyle s) {
  if (has_style(s, TextStyle::Quote)) return RGB(0,128,0);
  if (has_style(s, TextStyle::Math)) return RGB(107,35,142);
  if (has_style(s, TextStyle::Code)) return RGB(155,58,0);
  if (has_style(s, TextStyle::List)) return RGB(0,51,153);
  return RGB(32,32,32);
}

COLORREF style_bg(TextStyle s) {
  if (has_style(s, TextStyle::Selection)) return RGB(207,232,255);
  if (has_style(s, TextStyle::Code)) return RGB(247,241,227);
  if (has_style(s, TextStyle::Heading1) || has_style(s, TextStyle::Heading2) || has_style(s, TextStyle::Heading3)) return RGB(242,242,242);
  return RGB(255,255,255);
}
}

NativeWindowWin32::NativeWindowWin32(int w, int h) : width_(w), height_(h), input_(&registry_) {
  HINSTANCE inst = GetModuleHandleW(nullptr);
  WNDCLASSW wc{};
  wc.lpfnWndProc = &NativeWindowWin32::WndProc;
  wc.hInstance = inst;
  wc.lpszClassName = kClassName;
  wc.hCursor = load_system_cursor(kWinCursorArrow);
  wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
  RegisterClassW(&wc);

  const std::wstring title = widen(std::string("MDraft Win32 - v") + MDRAFT_VERSION);
  hwnd_ = CreateWindowExW(0, kClassName, title.c_str(),
                          WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                          CW_USEDEFAULT, CW_USEDEFAULT, width_, height_,
                          nullptr, nullptr, inst, this);
  if (!hwnd_) throw std::runtime_error("CreateWindowExW failed");
  menu_.attach(hwnd_, registry_);
  menu_.set_checked(CommandID::TogglePreview, preview_visible_);
  sync_preview_theme_menu();
  sync_preview_lock_menu();

  font_ = CreateFontW(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                      DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                      CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");
  HDC hdc = GetDC(hwnd_);
  HGDIOBJ old = SelectObject(hdc, font_);
  TEXTMETRICW tm{};
  GetTextMetricsW(hdc, &tm);
  const int measured_char_w = static_cast<int>(tm.tmAveCharWidth);
  const int measured_line_h = static_cast<int>(tm.tmHeight + tm.tmExternalLeading + 2);
  char_w_ = measured_char_w > 0 ? measured_char_w : 1;
  line_h_ = measured_line_h > 0 ? measured_line_h : 1;
  baseline_ = static_cast<int>(tm.tmAscent) + 1;
  SelectObject(hdc, old);
  ReleaseDC(hwnd_, hdc);
  metrics_.set_fixed_width(char_w_);
  metrics_.set_vertical_metrics(line_h_, baseline_);
}

NativeWindowWin32::~NativeWindowWin32() {
  if (font_) DeleteObject(font_);
}

LRESULT CALLBACK NativeWindowWin32::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
  NativeWindowWin32* self = reinterpret_cast<NativeWindowWin32*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
  if (msg == WM_NCCREATE) {
    auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
    self = reinterpret_cast<NativeWindowWin32*>(cs->lpCreateParams);
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    self->hwnd_ = hwnd;
  }
  return self ? self->handle(msg, wp, lp) : DefWindowProcW(hwnd, msg, wp, lp);
}

bool NativeWindowWin32::run(EditorRuntime& runtime) {
  runtime_ = &runtime;
  Win32ClipboardBridge clipboard(hwnd_);
  runtime.set_clipboard_bridge(&clipboard);
  invalidate();
  MSG msg{};
  while (running_ && GetMessageW(&msg, nullptr, 0, 0) > 0) {
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }
  runtime.set_clipboard_bridge(nullptr);
  return true;
}

void NativeWindowWin32::invalidate() {
  if (!hwnd_ || redraw_pending_) return;
  redraw_pending_ = true;
  InvalidateRect(hwnd_, nullptr, FALSE);
}

void NativeWindowWin32::repaint_now() {
  redraw_pending_ = false;
  InvalidateRect(hwnd_, nullptr, FALSE);
  UpdateWindow(hwnd_);
}

bool NativeWindowWin32::choose_markdown_path_for_open(std::string& out_path) {
  wchar_t file_name[MAX_PATH]{};
  OPENFILENAMEW ofn{};
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = hwnd_;
  ofn.lpstrFilter = L"Markdown-Dateien (*.md)\0*.md\0Textdateien (*.txt)\0*.txt\0Alle Dateien (*.*)\0*.*\0";
  ofn.lpstrFile = file_name;
  ofn.nMaxFile = MAX_PATH;
  ofn.lpstrDefExt = L"md";
  ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
  if (!GetOpenFileNameW(&ofn)) return false;
  out_path = narrow_utf8(file_name);
  return !out_path.empty();
}

bool NativeWindowWin32::choose_image_path_for_open(std::string& out_path) {
  wchar_t file_name[MAX_PATH]{};
  OPENFILENAMEW ofn{};
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = hwnd_;
  ofn.lpstrFilter = L"Bilddateien (*.png;*.jpg;*.jpeg;*.gif;*.webp;*.bmp;*.svg)\0*.png;*.jpg;*.jpeg;*.gif;*.webp;*.bmp;*.svg\0Alle Dateien (*.*)\0*.*\0";
  ofn.lpstrFile = file_name;
  ofn.nMaxFile = MAX_PATH;
  ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
  if (!GetOpenFileNameW(&ofn)) return false;
  out_path = narrow_utf8(file_name);
  return !out_path.empty();
}

bool NativeWindowWin32::choose_markdown_path_for_save(std::string& out_path) {
  wchar_t file_name[MAX_PATH]{};
  OPENFILENAMEW ofn{};
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = hwnd_;
  ofn.lpstrFilter = L"Markdown-Dateien (*.md)\0*.md\0Textdateien (*.txt)\0*.txt\0Alle Dateien (*.*)\0*.*\0";
  ofn.lpstrFile = file_name;
  ofn.nMaxFile = MAX_PATH;
  ofn.lpstrDefExt = L"md";
  ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
  if (!GetSaveFileNameW(&ofn)) return false;
  out_path = narrow_utf8(file_name);
  return !out_path.empty();
}

bool NativeWindowWin32::confirm_discard_if_dirty() {
  if (runtime_ == nullptr || !runtime_->state().dirty) return true;
  const int answer = MessageBoxW(hwnd_,
                                 L"Die aktuelle Datei hat ungespeicherte Änderungen. Trotzdem fortfahren?",
                                 L"MDraft",
                                 MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
  return answer == IDYES;
}

bool NativeWindowWin32::save_via_dialog() {
  if (runtime_ == nullptr) return false;
  std::string path;
  if (!choose_markdown_path_for_save(path)) {
    runtime_->state().status = "Speichern unter abgebrochen";
    repaint_now();
    return false;
  }
  runtime_->save_as(path);
  menu_.sync_enabled(*runtime_);
  repaint_now();
  return true;
}

bool NativeWindowWin32::choose_workspace_root(std::string& out_path) {
  BROWSEINFOW bi{};
  bi.hwndOwner = hwnd_;
  bi.lpszTitle = L"Workspace-Ordner auswählen";
  bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
  PIDLIST_ABSOLUTE pidl = SHBrowseForFolderW(&bi);
  if (pidl == nullptr) return false;

  wchar_t path[MAX_PATH]{};
  const BOOL ok = SHGetPathFromIDListW(pidl, path);
  CoTaskMemFree(pidl);
  if (!ok) return false;
  out_path = narrow_utf8(path);
  return !out_path.empty();
}

std::string NativeWindowWin32::current_search_query() const {
  if (runtime_ == nullptr) return {};
  const EditorState& st = runtime_->state();
  const std::string text = runtime_->buffer().str();

  if (st.selection.active && st.selection.begin() != st.selection.end()) {
    const std::size_t b = std::min(st.selection.begin(), text.size());
    const std::size_t e = std::min(st.selection.end(), text.size());
    if (b < e) return text.substr(b, std::min<std::size_t>(e - b, 120));
  }

  std::size_t b = std::min(st.cursor_byte, text.size());
  std::size_t e = b;
  while (b > 0) {
    const unsigned char c = static_cast<unsigned char>(text[b - 1]);
    if (!std::isalnum(c) && c != '_' && c != '-' && c != '.') break;
    --b;
  }
  while (e < text.size()) {
    const unsigned char c = static_cast<unsigned char>(text[e]);
    if (!std::isalnum(c) && c != '_' && c != '-' && c != '.') break;
    ++e;
  }
  if (b < e) return text.substr(b, std::min<std::size_t>(e - b, 120));
  return {};
}

void NativeWindowWin32::open_workspace(const std::string& path) {
  workspace_index_ = build_workspace_index(path);
  runtime_->state().status = workspace_summary(workspace_index_);
  MessageBoxW(hwnd_, widen(runtime_->state().status).c_str(),
              L"MDraft - Workspace indiziert",
              MB_OK | (workspace_index_.diagnostics.empty() ? MB_ICONINFORMATION : MB_ICONWARNING));
  repaint_now();
}

void NativeWindowWin32::reindex_workspace() {
  if (workspace_index_.root.path.empty()) {
    std::string path;
    if (!choose_workspace_root(path)) {
      runtime_->state().status = "Workspace öffnen abgebrochen";
      repaint_now();
      return;
    }
    open_workspace(path);
    return;
  }
  open_workspace(workspace_index_.root.path);
}

void NativeWindowWin32::search_workspace_from_editor() {
  if (workspace_index_.root.path.empty()) {
    std::string path;
    if (!choose_workspace_root(path)) {
      runtime_->state().status = "Workspace-Suche abgebrochen: kein Workspace";
      repaint_now();
      return;
    }
    open_workspace(path);
  }

  const std::string query = current_search_query();
  if (query.empty()) {
    runtime_->state().status = "Workspace-Suche: Text markieren oder Cursor auf Suchwort setzen";
    MessageBoxW(hwnd_, L"Bitte Text markieren oder Cursor auf ein Suchwort setzen.",
                L"MDraft - Workspace-Suche",
                MB_OK | MB_ICONINFORMATION);
    repaint_now();
    return;
  }

  SearchOptions options;
  options.max_results = 128;
  options.context_chars = 80;
  const std::vector<SearchResult> hits = search_workspace(workspace_index_, query, options);
  const std::string result = format_search_results(hits, query, 14);
  runtime_->state().status = "Workspace-Suche: " + std::to_string(hits.size()) + " Treffer für '" + query + "'";
  MessageBoxW(hwnd_, widen(result).c_str(),
              L"MDraft - Multi-Datei-Suche",
              MB_OK | (hits.empty() ? MB_ICONINFORMATION : MB_ICONASTERISK));
  repaint_now();
}

bool NativeWindowWin32::open_workspace_symbol(const MarkdownSymbol& symbol) {
  if (runtime_ == nullptr || workspace_index_.root.path.empty() || symbol.file.empty()) return false;

  const std::string full = normalize_workspace_path(workspace_index_.root.path + "/" + symbol.file);
  const std::string text = read_file(full);
  runtime_->open_document(full, text);
  runtime_->state().cursor_byte = std::min(symbol.byte_offset, runtime_->buffer().size());
  runtime_->state().selection.active = false;
  cursor_.sync(runtime_->state(), runtime_->buffer().str());
  const int editor_h = height_ - menu_h_ - 28;
  viewport_.ensure_cursor_visible(runtime_->state(), runtime_->buffer().str(),
                                  editor_area_width() - 36, editor_h, char_w_, line_h_);
  runtime_->state().status = "Symbol: " + symbol.file + " :: " + symbol.title;
  menu_.sync_enabled(*runtime_);
  repaint_now();
  return true;
}

void NativeWindowWin32::navigate_symbol_from_editor() {
  if (runtime_ == nullptr) return;
  if (workspace_index_.root.path.empty()) {
    std::string path;
    if (!choose_workspace_root(path)) {
      runtime_->state().status = "Symbol-Navigation abgebrochen: kein Workspace";
      repaint_now();
      return;
    }
    open_workspace(path);
  }

  const std::string query = current_search_query();
  SymbolQueryOptions options;
  options.max_results = query.empty() ? 256 : 64;
  const std::vector<MarkdownSymbol> hits = find_workspace_symbols(workspace_index_, query, options);

  if (hits.empty()) {
    const std::string msg = query.empty()
      ? "Keine Markdown-Symbole im Workspace gefunden."
      : "Kein Symbol gefunden für: " + query;
    runtime_->state().status = "Symbol-Navigation: 0 Treffer";
    MessageBoxW(hwnd_, widen(msg).c_str(),
                L"MDraft - Symbol-Navigation",
                MB_OK | MB_ICONINFORMATION);
    repaint_now();
    return;
  }

  const std::string list = format_symbol_results(hits, query, 32);
  if (query.empty()) {
    runtime_->state().status = "Symbol-Navigation: " + std::to_string(hits.size()) + " Symbole";
    MessageBoxW(hwnd_, widen(list).c_str(),
                L"MDraft - Symbole im Workspace",
                MB_OK | MB_ICONINFORMATION);
    repaint_now();
    return;
  }

  open_workspace_symbol(hits.front());
  MessageBoxW(hwnd_, widen(list + "\n\nGeöffnet: erster Treffer").c_str(),
              L"MDraft - Symbol-Navigation",
              MB_OK | MB_ICONASTERISK);
}


void NativeWindowWin32::validate_links_from_workspace() {
  if (runtime_ == nullptr) return;
  if (workspace_index_.root.path.empty()) {
    std::string path;
    if (!choose_workspace_root(path)) {
      runtime_->state().status = "Link-Validierung abgebrochen: kein Workspace";
      repaint_now();
      return;
    }
    open_workspace(path);
  }

  const LinkValidationReport report = validate_workspace_links(workspace_index_);
  const std::string msg = format_link_validation_report(report, 48);
  runtime_->state().status = "Link-Validierung: " + std::to_string(report.links_unresolved) + " offene Links";
  MessageBoxW(hwnd_, widen(msg).c_str(),
              L"MDraft - Link-Validierung",
              MB_OK | (report.links_unresolved == 0 ? MB_ICONINFORMATION : MB_ICONWARNING));
  repaint_now();
}

void NativeWindowWin32::suggest_images_from_editor() {
  if (runtime_ == nullptr) return;
  if (workspace_index_.root.path.empty()) {
    std::string path;
    if (!choose_workspace_root(path)) {
      runtime_->state().status = "Bildpfad-Vorschläge abgebrochen: kein Workspace";
      repaint_now();
      return;
    }
    open_workspace(path);
  }

  std::string current_file;
  const std::string current_path = normalize_workspace_path(runtime_->path());
  const std::string root_path = normalize_workspace_path(workspace_index_.root.path);
  if (!current_path.empty() && current_path.rfind(root_path + "/", 0) == 0) {
    current_file = current_path.substr(root_path.size() + 1);
  }

  const std::string query = current_search_query();
  ImageSuggestionOptions options;
  options.max_results = 32;
  const std::vector<ImageSuggestion> suggestions = suggest_workspace_images(workspace_index_, current_file, query, options);
  const std::string msg = format_image_suggestions(suggestions, query, 32);
  runtime_->state().status = "Bildpfad-Vorschläge: " + std::to_string(suggestions.size()) + " Treffer";
  MessageBoxW(hwnd_, widen(msg).c_str(),
              L"MDraft - Bildpfad-Vorschläge",
              MB_OK | (suggestions.empty() ? MB_ICONWARNING : MB_ICONINFORMATION));
  repaint_now();
}


void NativeWindowWin32::run_diagnostics_from_workspace() {
  if (runtime_ == nullptr) return;
  if (workspace_index_.root.path.empty()) {
    std::string path;
    if (!choose_workspace_root(path)) {
      runtime_->state().status = "Diagnostics/Linting abgebrochen: kein Workspace";
      repaint_now();
      return;
    }
    open_workspace(path);
  } else {
    reindex_workspace();
  }

  const LintReport report = lint_workspace(workspace_index_);
  const std::string msg = format_lint_report(report, 80);
  runtime_->state().status =
    "Diagnostics/Linting: errors=" + std::to_string(report.error_count) +
    " warnings=" + std::to_string(report.warning_count) +
    " infos=" + std::to_string(report.info_count);

  UINT icon = MB_ICONINFORMATION;
  if (report.error_count > 0) icon = MB_ICONERROR;
  else if (report.warning_count > 0) icon = MB_ICONWARNING;

  MessageBoxW(hwnd_, widen(msg).c_str(),
              L"MDraft - Diagnostics/Linting",
              MB_OK | icon);
  repaint_now();
}

void NativeWindowWin32::show_git_status_light() {
  if (runtime_ == nullptr) return;

  std::string hint;
  if (!workspace_index_.root.path.empty()) {
    hint = workspace_index_.root.path;
  } else if (!runtime_->path().empty() && runtime_->path() != "<untitled>") {
    hint = extract_directory_head(runtime_->path());
  } else {
    hint = ".";
  }

  const GitStatusReport report = read_git_status(hint);
  const std::string msg = format_git_status_report(report, 120);
  if (report.repository) {
    runtime_->state().status =
      "Git: files=" + std::to_string(report.files.size()) +
      " modified=" + std::to_string(report.modified_count) +
      " untracked=" + std::to_string(report.untracked_count) +
      " conflicts=" + std::to_string(report.conflict_count);
  } else {
    runtime_->state().status = "Git: kein Repository oder git.exe nicht gefunden";
  }

  UINT icon = MB_ICONINFORMATION;
  if (!report.repository) icon = MB_ICONWARNING;
  else if (report.conflict_count > 0) icon = MB_ICONERROR;
  else if (!report.files.empty()) icon = MB_ICONWARNING;

  MessageBoxW(hwnd_, widen(msg).c_str(),
              L"MDraft - Git-Status light",
              MB_OK | icon);
  repaint_now();
}

void NativeWindowWin32::insert_snippet_from_editor() {
  if (runtime_ == nullptr) return;
  EditorState& st = runtime_->state();
  const std::string text = runtime_->buffer().str();

  std::string trigger;
  std::size_t begin = st.cursor_byte;
  std::size_t end = st.cursor_byte;
  if (st.selection.active && st.selection.begin() != st.selection.end()) {
    begin = std::min(st.selection.begin(), text.size());
    end = std::min(st.selection.end(), text.size());
    if (begin < end) trigger = text.substr(begin, end - begin);
  } else {
    const auto bounds = word_bounds_for_snippet(text, st.cursor_byte);
    begin = bounds.first;
    end = bounds.second;
    if (begin < end) trigger = text.substr(begin, end - begin);
  }

  const SnippetExpansion expansion = expand_snippet_trigger(trigger);
  if (!expansion.matched) {
    const std::string catalog = format_snippet_catalog() +
      "\n\nVerwendung: Trigger schreiben, z.B. table oder mermaid, dann Ctrl+J.";
    MessageBoxW(hwnd_, widen(catalog).c_str(), L"MDraft - Snippets", MB_OK | MB_ICONINFORMATION);
    st.status = trigger.empty() ? "Snippet: kein Trigger unter Cursor" : "Snippet unbekannt: " + trigger;
    repaint_now();
    return;
  }

  st.selection.active = true;
  st.selection.anchor = begin;
  st.selection.focus = end;
  runtime_->insert_text(expansion.body);
  st.cursor_byte = begin + std::min<std::size_t>(expansion.cursor_offset, expansion.body.size());
  st.selection.active = false;
  st.status = "Snippet eingefügt: " + expansion.trigger;
  repaint_now();
}

void NativeWindowWin32::insert_image_from_dialog() {
  if (runtime_ == nullptr) return;
  std::string image_path;
  if (!choose_image_path_for_open(image_path)) {
    runtime_->state().status = "Bild einfügen abgebrochen";
    repaint_now();
    return;
  }

  const std::string href = make_relative_asset_href(runtime_->path(), image_path);
  std::string alt = current_search_query();
  if (alt.empty()) {
    alt = extract_filename_tail(image_path);
    const std::size_t dot = alt.find_last_of('.');
    if (dot != std::string::npos) alt = alt.substr(0, dot);
  }
  runtime_->insert_text(make_markdown_image(alt, href) + "\n");
  runtime_->state().status = "Bild eingefügt: " + href;
  repaint_now();
}

void NativeWindowWin32::execute(CommandID id) {
  if (!runtime_ || id == CommandID::NoCommand) return;

  if (id == CommandID::TogglePreview) {
    preview_visible_ = !preview_visible_;
    if (!preview_visible_) preview_focused_ = false;
    preview_scroll_y_ = 0;
    menu_.set_checked(CommandID::TogglePreview, preview_visible_);
    runtime_->state().status = preview_visible_ ? "RenderIR/HTML Live-Vorschau aktiv" : "RenderIR/HTML Live-Vorschau aus";
    repaint_now();
    return;
  }

  if (id == CommandID::TogglePreviewLock) {
    toggle_preview_lock();
    return;
  }

  switch (id) {
    case CommandID::PreviewThemeStandard: set_preview_theme(HtmlTheme::Standard); return;
    case CommandID::PreviewThemeGitHub: set_preview_theme(HtmlTheme::GitHub); return;
    case CommandID::PreviewThemeCyberpunk: set_preview_theme(HtmlTheme::Cyberpunk); return;
    case CommandID::PreviewThemeDystopia: set_preview_theme(HtmlTheme::Dystopia); return;
    case CommandID::PreviewThemeHorror: set_preview_theme(HtmlTheme::Horror); return;
    case CommandID::PreviewThemeAdventure: set_preview_theme(HtmlTheme::Adventure); return;
    default: break;
  }

  if (id == CommandID::OpenWorkspace) {
    std::string path;
    if (!choose_workspace_root(path)) {
      runtime_->state().status = "Workspace öffnen abgebrochen";
      repaint_now();
      return;
    }
    open_workspace(path);
    return;
  }

  if (id == CommandID::ReindexWorkspace) {
    reindex_workspace();
    return;
  }

  if (id == CommandID::WorkspaceStatus) {
    const std::string msg = workspace_summary(workspace_index_);
    runtime_->state().status = msg;
    MessageBoxW(hwnd_, widen(msg).c_str(), L"MDraft - Workspace-Status", MB_OK | MB_ICONINFORMATION);
    repaint_now();
    return;
  }

  if (id == CommandID::WorkspaceSearch) {
    search_workspace_from_editor();
    return;
  }

  if (id == CommandID::WorkspaceSymbols) {
    navigate_symbol_from_editor();
    return;
  }

  if (id == CommandID::ValidateLinks) {
    validate_links_from_workspace();
    return;
  }

  if (id == CommandID::ImagePathSuggestions) {
    suggest_images_from_editor();
    return;
  }

  if (id == CommandID::RunDiagnostics) {
    run_diagnostics_from_workspace();
    return;
  }

  if (id == CommandID::GitStatus) {
    show_git_status_light();
    return;
  }

  if (id == CommandID::InsertSnippet) {
    insert_snippet_from_editor();
    return;
  }

  if (id == CommandID::InsertImage) {
    insert_image_from_dialog();
    return;
  }

  try {
    if (id == CommandID::NewFile) {
      if (!confirm_discard_if_dirty()) {
        runtime_->state().status = "Neu abgebrochen";
        repaint_now();
        return;
      }
      runtime_->new_document();
      menu_.sync_enabled(*runtime_);
      repaint_now();
      return;
    }

    if (id == CommandID::OpenFile) {
      if (!confirm_discard_if_dirty()) {
        runtime_->state().status = "Öffnen abgebrochen";
        repaint_now();
        return;
      }
      std::string path;
      if (!choose_markdown_path_for_open(path)) {
        runtime_->state().status = "Öffnen abgebrochen";
        repaint_now();
        return;
      }
      runtime_->open_document(path, read_file(path));
      menu_.sync_enabled(*runtime_);
      repaint_now();
      return;
    }

    if (id == CommandID::SaveAs) {
      save_via_dialog();
      return;
    }

    if (id == CommandID::Save && runtime_->path().empty()) {
      save_via_dialog();
      return;
    }

    bool run_flag = running_;
    runtime_->execute(id, run_flag);
    running_ = run_flag;
    if (!running_) PostQuitMessage(0);
    if (runtime_) menu_.sync_enabled(*runtime_);
    repaint_now();
  } catch (const std::exception& e) {
    runtime_->state().status = std::string("Fehler: ") + e.what();

    if (id == CommandID::Save) {
      std::wstring msg = widen(runtime_->state().status);
      msg += L"\n\nSpeichern unter öffnen?";
      const int answer = MessageBoxW(hwnd_, msg.c_str(),
                                     L"MDraft - Speichern fehlgeschlagen",
                                     MB_YESNO | MB_ICONERROR | MB_DEFBUTTON1);
      if (answer == IDYES) {
        try {
          save_via_dialog();
          return;
        } catch (const std::exception& e2) {
          runtime_->state().status = std::string("Speichern unter fehlgeschlagen: ") + e2.what();
        }
      }
    } else {
      MessageBoxW(hwnd_, widen(runtime_->state().status).c_str(),
                  L"MDraft - Befehl fehlgeschlagen",
                  MB_OK | MB_ICONERROR);
    }

    if (runtime_) menu_.sync_enabled(*runtime_);
    repaint_now();
  }
}


LRESULT NativeWindowWin32::handle(UINT msg, WPARAM wp, LPARAM lp) {
  switch (msg) {
    case WM_SIZE:
      width_ = LOWORD(lp);
      height_ = HIWORD(lp);
      invalidate();
      return 0;
    case WM_ERASEBKGND:
      return 1;
    case WM_INITMENUPOPUP:
      if (runtime_) menu_.sync_enabled(*runtime_);
      return 0;
    case WM_COMMAND: {
      const CommandID id = menu_.command_from_wparam(wp);
      if (id != CommandID::NoCommand) {
        execute(id);
        return 0;
      }
      break;
    }
    case WM_SETCURSOR: {
      if (LOWORD(lp) == HTCLIENT) {
        POINT p{};
        GetCursorPos(&p);
        ScreenToClient(hwnd_, &p);
        SetCursor(load_system_cursor(p.y < menu_h_ ? kWinCursorArrow : kWinCursorIBeam));
        return TRUE;
      }
      break;
    }
    case WM_PAINT: {
      PAINTSTRUCT ps{};
      HDC hdc = BeginPaint(hwnd_, &ps);
      HDC mem = CreateCompatibleDC(hdc);
      HBITMAP bmp = CreateCompatibleBitmap(hdc, width_, height_);
      HGDIOBJ old_bmp = SelectObject(mem, bmp);
      redraw_pending_ = false;
      draw(mem);
      BitBlt(hdc, 0, 0, width_, height_, mem, 0, 0, SRCCOPY);
      SelectObject(mem, old_bmp);
      DeleteObject(bmp);
      DeleteDC(mem);
      EndPaint(hwnd_, &ps);
      return 0;
    }
    case WM_KEYDOWN:
      handle_key(wp, lp);
      return 0;
    case WM_CHAR:
      handle_char(wp);
      return 0;
    case WM_LBUTTONDOWN:
      SetCapture(hwnd_);
      handle_mouse_down(GET_X_LPARAM(lp), GET_Y_LPARAM(lp), false);
      return 0;
    case WM_RBUTTONDOWN:
      handle_mouse_down(GET_X_LPARAM(lp), GET_Y_LPARAM(lp), true);
      return 0;
    case WM_MOUSEMOVE:
      handle_mouse_move(GET_X_LPARAM(lp), GET_Y_LPARAM(lp), wp);
      return 0;
    case WM_LBUTTONUP:
      ReleaseCapture();
      handle_mouse_up();
      return 0;
    case WM_MOUSEWHEEL: {
      POINT p{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
      ScreenToClient(hwnd_, &p);
      handle_wheel(GET_WHEEL_DELTA_WPARAM(wp), p.x, p.y);
      return 0;
    }
    case WM_DESTROY:
      running_ = false;
      PostQuitMessage(0);
      return 0;
    default:
      return DefWindowProcW(hwnd_, msg, wp, lp);
  }
  return DefWindowProcW(hwnd_, msg, wp, lp);
}

void NativeWindowWin32::handle_key(WPARAM vk, LPARAM) {
  if (!runtime_) return;
  const bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
  const bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;

  if (ctrl) {
    char key = 0;
    if (vk >= 'A' && vk <= 'Z') key = static_cast<char>(std::tolower(static_cast<int>(vk)));
    if (key) {
      CommandID cid = registry_.lookup({static_cast<unsigned int>(key), true, shift, false});
      if (cid != CommandID::NoCommand) {
        execute(cid);
        return;
      }
    }
  }

  if (preview_visible_ && preview_focused_) {
    const int page = std::max(line_h_ * 3, height_ - menu_h_ - 96);
    bool consumed = true;
    switch (vk) {
      case VK_UP: preview_scroll_y_ -= line_h_; break;
      case VK_DOWN: preview_scroll_y_ += line_h_; break;
      case VK_PRIOR: preview_scroll_y_ -= page; break;
      case VK_NEXT: preview_scroll_y_ += page; break;
      case VK_HOME: preview_scroll_y_ = 0; break;
      case VK_END: preview_scroll_y_ = preview_content_h_; break;
      default: consumed = false; break;
    }
    if (consumed) {
      clamp_preview_scroll();
      runtime_->state().status = "Preview scroll=" + std::to_string(preview_scroll_y_);
      invalidate();
      return;
    }
  }

  InputEvent ev{};
  ev.ctrl = ctrl;
  ev.shift = shift;
  switch (vk) {
    case VK_LEFT: ev.key = InputKey::Left; break;
    case VK_RIGHT: ev.key = InputKey::Right; break;
    case VK_UP: ev.key = InputKey::Up; break;
    case VK_DOWN: ev.key = InputKey::Down; break;
    case VK_PRIOR: ev.key = InputKey::PageUp; break;
    case VK_NEXT: ev.key = InputKey::PageDown; break;
    case VK_BACK: ev.key = InputKey::Backspace; break;
    case VK_DELETE: ev.key = InputKey::DeleteKey; break;
    case VK_RETURN: ev.key = InputKey::Enter; break;
    case VK_ESCAPE: ev.key = InputKey::Escape; break;
    default: return;
  }

  EditorAction a = input_.translate(ev, runtime_->state().mode == EditorMode::Insert);
  const std::string text = runtime_->buffer().str();
  const int editor_h = height_ - menu_h_ - 28;
  switch (a.kind) {
    case ActionKind::Command: execute(a.command); break;
    case ActionKind::MoveLeft: cursor_.move_left(runtime_->state(), text, a.select); break;
    case ActionKind::MoveRight: cursor_.move_right(runtime_->state(), text, a.select); break;
    case ActionKind::MoveUp: cursor_.move_up(runtime_->state(), text, a.select); break;
    case ActionKind::MoveDown: cursor_.move_down(runtime_->state(), text, a.select); break;
    case ActionKind::PageUp: viewport_.scroll_lines(runtime_->state(), -std::max(1, editor_h / line_h_), text); break;
    case ActionKind::PageDown: viewport_.scroll_lines(runtime_->state(), std::max(1, editor_h / line_h_), text); break;
    case ActionKind::Backspace: runtime_->backspace(); break;
    case ActionKind::DeleteAt: runtime_->delete_at(); break;
    case ActionKind::SmartNewline: runtime_->smart_newline(); break;
    default: break;
  }
  viewport_.ensure_cursor_visible(runtime_->state(), runtime_->buffer().str(), editor_area_width() - 36, editor_h, char_w_, line_h_);
  invalidate();
}

void NativeWindowWin32::handle_char(WPARAM ch) {
  if (!runtime_) return;
  if (preview_visible_ && preview_focused_) return;
  if ((GetKeyState(VK_CONTROL) & 0x8000) != 0) return;
  if (ch < 32 || ch == 127) return;
  char utf8[8]{};
  if (ch < 128) {
    utf8[0] = static_cast<char>(ch);
    runtime_->insert_text(std::string(utf8, 1));
  } else {
    wchar_t wc = static_cast<wchar_t>(ch);
    int n = WideCharToMultiByte(CP_UTF8, 0, &wc, 1, utf8, 8, nullptr, nullptr);
    if (n > 0) runtime_->insert_text(std::string(utf8, static_cast<std::size_t>(n)));
  }
  viewport_.ensure_cursor_visible(runtime_->state(), runtime_->buffer().str(), editor_area_width() - 36, height_ - menu_h_ - 28, char_w_, line_h_);
  invalidate();
}


void NativeWindowWin32::handle_mouse_down(int x, int y, bool right) {
  if (!runtime_) return;
  SetFocus(hwnd_);
  if (preview_visible_ && x >= preview_area_x()) {
    runtime_->state().status = "RenderIR Preview: Anzeige aus DisplayList";
    invalidate();
    return;
  }
  const std::string text = runtime_->buffer().str();
  std::size_t b = mouse_to_byte(x, y, text, runtime_->state());
  if (right) {
    runtime_->state().cursor_byte = b;
    runtime_->state().status = "Kontext: byte " + std::to_string(b);
  } else {
    selection_.begin_drag(runtime_->state(), b);
    mouse_selecting_ = true;
  }
  cursor_.sync(runtime_->state(), text);
  invalidate();
}

void NativeWindowWin32::handle_mouse_move(int x, int y, WPARAM flags) {
  if (!runtime_ || !mouse_selecting_ || !(flags & MK_LBUTTON)) return;
  const std::string text = runtime_->buffer().str();
  selection_.update_drag(runtime_->state(), mouse_to_byte(x, y, text, runtime_->state()));
  invalidate();
}

void NativeWindowWin32::handle_mouse_up() {
  if (!runtime_) return;
  selection_.end_drag(runtime_->state());
  mouse_selecting_ = false;
  invalidate();
}

void NativeWindowWin32::handle_wheel(short delta, int x, int y) {
  if (!runtime_) return;
  if (point_in_preview(x, y)) {
    preview_focused_ = true;
    preview_scroll_y_ += (delta > 0 ? -3 : 3) * line_h_;
    clamp_preview_scroll();
    runtime_->state().status = "Preview scroll=" + std::to_string(preview_scroll_y_);
    invalidate();
    return;
  }

  preview_focused_ = false;
  const std::string text = runtime_->buffer().str();
  viewport_.scroll_lines(runtime_->state(), delta > 0 ? -3 : 3, text);
  invalidate();
}

std::size_t NativeWindowWin32::mouse_to_byte(int x, int y, const std::string& text, const EditorState& state) const {
  const int top = menu_h_ + 6;
  const int text_x = 8 + 48;
  const int text_y = top;
  const int row = state.scroll_row + std::max(0, (y - text_y) / line_h_);
  const int col = state.scroll_col + std::max(0, (x - text_x) / char_w_);
  LineIndex idx = build_line_index(text);
  return row_col_to_byte(idx, row, col);
}

void NativeWindowWin32::fill_rect(HDC hdc, int x, int y, int w, int h, COLORREF color) {
  RECT r{x, y, x + std::max(0, w), y + std::max(0, h)};
  HBRUSH br = CreateSolidBrush(color);
  FillRect(hdc, &r, br);
  DeleteObject(br);
}

void NativeWindowWin32::draw_text(HDC hdc, int x, int y, const std::string& s, COLORREF fg, COLORREF bg, bool fill_bg) {
  SetTextColor(hdc, fg);
  SetBkColor(hdc, bg);
  SetBkMode(hdc, fill_bg ? OPAQUE : TRANSPARENT);
  std::wstring ws = widen(s);
  TextOutW(hdc, x, y, ws.c_str(), static_cast<int>(ws.size()));
}

int NativeWindowWin32::editor_area_width() const noexcept {
  if (!preview_visible_) return width_;
  return std::max(420, (width_ * 52) / 100);
}

int NativeWindowWin32::preview_area_x() const noexcept {
  return editor_area_width();
}

bool NativeWindowWin32::point_in_preview(int x, int y) const noexcept {
  return preview_visible_ && x >= preview_area_x() && y >= menu_h_ && y < height_ - 24;
}

void NativeWindowWin32::clamp_preview_scroll() noexcept {
  const int header_h = menu_h_ + 56;
  const int viewport_top = header_h + 10;
  const int viewport_bottom = std::max(viewport_top + 1, height_ - 24 - 8);
  const int viewport_h = std::max(1, viewport_bottom - viewport_top);
  const int max_scroll = std::max(0, preview_content_h_ - viewport_h);
  preview_scroll_y_ = std::max(0, std::min(preview_scroll_y_, max_scroll));
}

void NativeWindowWin32::sync_preview_theme_menu() {
  menu_.set_checked(CommandID::PreviewThemeStandard, preview_theme_ == HtmlTheme::Standard);
  menu_.set_checked(CommandID::PreviewThemeGitHub, preview_theme_ == HtmlTheme::GitHub);
  menu_.set_checked(CommandID::PreviewThemeCyberpunk, preview_theme_ == HtmlTheme::Cyberpunk);
  menu_.set_checked(CommandID::PreviewThemeDystopia, preview_theme_ == HtmlTheme::Dystopia);
  menu_.set_checked(CommandID::PreviewThemeHorror, preview_theme_ == HtmlTheme::Horror);
  menu_.set_checked(CommandID::PreviewThemeAdventure, preview_theme_ == HtmlTheme::Adventure);
}

void NativeWindowWin32::set_preview_theme(HtmlTheme theme) {
  preview_theme_ = theme;
  preview_scroll_y_ = 0;
  sync_preview_theme_menu();
  if (runtime_) runtime_->state().status = std::string("Preview-Thema: ") + html_theme_label(theme);
  repaint_now();
}

void NativeWindowWin32::sync_preview_lock_menu() {
  menu_.set_checked(CommandID::TogglePreviewLock, preview_locked_);
}

void NativeWindowWin32::toggle_preview_lock() {
  if (!runtime_) return;
  preview_locked_ = !preview_locked_;
  if (preview_locked_) {
    locked_preview_path_ = runtime_->path();
    locked_preview_markdown_ = runtime_->buffer().str();
    preview_visible_ = true;
    preview_focused_ = true;
    menu_.set_checked(CommandID::TogglePreview, preview_visible_);
    runtime_->state().status =
      std::string("Preview gesperrt: ") +
      (locked_preview_path_.empty() ? std::string("<unbenannt>") : locked_preview_path_);
  } else {
    locked_preview_path_.clear();
    locked_preview_markdown_.clear();
    runtime_->state().status = "Preview entsperrt: folgt aktuellem Editor";
  }
  sync_preview_lock_menu();
  clamp_preview_scroll();
  repaint_now();
}

std::string NativeWindowWin32::preview_markdown_source() const {
  if (preview_locked_) {
    return locked_preview_markdown_;
  }
  return runtime_->buffer().str();
}

std::string NativeWindowWin32::preview_source_label() const {
  if (!preview_locked_) {
    return runtime_ == nullptr || runtime_->path().empty() ? std::string("<live: unbenannt>") : std::string("<live: ") + runtime_->path() + ">";
  }
  return locked_preview_path_.empty() ? std::string("<locked: unbenannt>") : std::string("<locked: ") + locked_preview_path_ + ">";
}

void NativeWindowWin32::draw_editor(HDC hdc, int editor_w) {
  if (!runtime_) return;
  const std::string text = runtime_->buffer().str();
  const EditorState& state = runtime_->state();
  LineIndex idx = build_line_index(text);
  AttributeBuffer attrs = highlighter_.build(text, state.selection);

  fill_rect(hdc, 0, 0, editor_w, height_, RGB(255,255,255));

  const int status_h = 24;
  const int top = menu_h_ + 6;
  const int left = 8;
  const int text_x = left + 48;
  const int text_y = top;
  const int editor_h = height_ - top - status_h - 8;
  Viewport v = viewport_.visible(state, editor_w - 36, editor_h, char_w_, line_h_);

  for (int vr = 0; vr < v.rows; ++vr) {
    int row = v.first_row + vr;
    if (row >= static_cast<int>(idx.starts.size())) break;
    std::size_t e = idx.ends[static_cast<std::size_t>(row)];
    int y = text_y + vr * line_h_;

    char rn[16];
    std::snprintf(rn, sizeof(rn), "%4d", row + 1);
    draw_text(hdc, left, y, rn, RGB(144,144,144), RGB(255,255,255), false);

    std::size_t bi = row_col_to_byte(idx, row, v.first_col);
    int x = text_x;
    while (bi < e && x < editor_w - 24) {
      const std::size_t glyph_len = std::max<std::size_t>(1, utf8_codepoint_len(text, bi, e));
      std::string glyph = text.substr(bi, glyph_len);
      if (glyph == "\t") glyph = " ";
      TextStyle st = attrs.at(bi);
      COLORREF bg = style_bg(st);
      if (bg != RGB(255,255,255)) fill_rect(hdc, x, y, char_w_, line_h_, bg);
      draw_text(hdc, x, y, glyph, style_fg(st), bg, false);
      if (has_style(st, TextStyle::Bold)) draw_text(hdc, x + 1, y, glyph, style_fg(st), bg, false);
      x += char_w_;
      bi += glyph_len;
    }
  }

  Viewport cv = viewport_.visible(state, editor_w - 36, editor_h, char_w_, line_h_);
  CursorVisual cur = cursor_visual_.visual_for_byte(text, state, cv, metrics_, text_x, text_y);
  if (cur.visible && cur.x < editor_w - 8) fill_rect(hdc, cur.x, cur.y, 2, cur.h, RGB(0,0,0));
}

void NativeWindowWin32::draw_render_preview(HDC hdc, int x0, int w) {
  if (!runtime_ || w <= 80) return;

  const int header_h = menu_h_ + 56;
  const int status_h = 24;
  const int viewport_top = header_h + 10;
  const int viewport_bottom = std::max(viewport_top + 1, height_ - status_h - 8);
  const int viewport_h = viewport_bottom - viewport_top;

  const bool github_theme = preview_theme_ == HtmlTheme::GitHub;
  const COLORREF preview_bg = github_theme ? RGB(255,255,255) : RGB(7,4,4);
  const COLORREF preview_border = github_theme ? RGB(208,215,222) : RGB(76,23,23);
  const COLORREF header_bg = github_theme ? RGB(246,248,250) : RGB(23,8,8);
  const COLORREF header_fg = github_theme ? RGB(36,41,47) : RGB(234,223,218);
  const COLORREF meta_fg = github_theme ? RGB(87,96,106) : RGB(162,143,139);
  const COLORREF thumb = github_theme ? RGB(9,105,218) : RGB(208,24,24);

  fill_rect(hdc, x0, 0, w, height_, preview_bg);
  fill_rect(hdc, x0, 0, 1, height_, preview_border);
  fill_rect(hdc, x0 + 1, 0, w - 1, header_h, header_bg);
  draw_text(hdc, x0 + 18, menu_h_ + 8,
            std::string("RenderIR/HTML Live-Vorschau · ") + html_theme_label(preview_theme_) + (preview_locked_ ? " · LOCKED" : ""),
            header_fg, header_bg, false);

  const int list_w = std::min(980, std::max(320, w - 72));
  const std::string preview_markdown = preview_markdown_source();
  const std::string preview_base = preview_locked_ ? locked_preview_path_ : runtime_->path();
  DisplayList list = markdown_to_display_list_with_base(preview_markdown, preview_theme_, list_w, preview_base);
  preview_content_h_ = list.height + 20;
  clamp_preview_scroll();

  const std::string meta = truncate_middle(
    "commands=" + std::to_string(list.commands.size()) +
    " math=" + std::to_string(list.math_box_count) +
    " mermaid=" + std::to_string(list.mermaid_node_count) + "/" +
    std::to_string(list.mermaid_edge_count) +
    " scroll=" + std::to_string(preview_scroll_y_) +
    " hash=" + std::to_string(display_list_hash(list)) +
    " source=" + preview_source_label(),
    static_cast<std::size_t>(std::max(24, (w - 330) / std::max(1, char_w_))));
  draw_text(hdc, x0 + 18, menu_h_ + 30, meta, meta_fg, header_bg, false);

  const int ox = x0 + std::max(20, (w - list.width - 16) / 2);
  const int oy = viewport_top - preview_scroll_y_;

  const int saved_dc = SaveDC(hdc);
  IntersectClipRect(hdc, x0 + 1, viewport_top, x0 + w - 1, viewport_bottom);
  display_list_to_gdi(hdc, list, ox, oy);
  RestoreDC(hdc, saved_dc);

  const int track_x = x0 + w - 12;
  const int track_w = 6;
  fill_rect(hdc, track_x, viewport_top, track_w, viewport_h, header_bg);
  const int max_scroll = std::max(0, preview_content_h_ - viewport_h);
  if (max_scroll > 0) {
    const int thumb_h = std::max(28, (viewport_h * viewport_h) / std::max(viewport_h, preview_content_h_));
    const int thumb_y = viewport_top + ((viewport_h - thumb_h) * preview_scroll_y_) / max_scroll;
    fill_rect(hdc, track_x, thumb_y, track_w, thumb_h, thumb);
  } else {
    fill_rect(hdc, track_x, viewport_top, track_w, viewport_h, preview_border);
  }

  if (preview_focused_) {
    fill_rect(hdc, x0 + 4, viewport_top, 3, viewport_h, thumb);
  }
}

void NativeWindowWin32::draw(HDC hdc) {
  if (!runtime_) return;
  HGDIOBJ old_font = SelectObject(hdc, font_);

  fill_rect(hdc, 0, 0, width_, height_, RGB(255,255,255));
  const int editor_w = editor_area_width();
  draw_editor(hdc, editor_w);
  if (preview_visible_) {
    draw_render_preview(hdc, preview_area_x(), width_ - preview_area_x());
  }

  const EditorState& state = runtime_->state();
  const int status_h = 24;
  fill_rect(hdc, 0, height_ - status_h, width_, status_h, RGB(240,240,240));
  std::string mode = state.mode == EditorMode::Insert ? "INSERT" : "COMMAND";
  std::string st = " " + mode + (state.dirty ? " * " : "   ") +
                   " byte=" + std::to_string(state.cursor_byte) +
                   (preview_visible_ ? (preview_locked_ ? "  preview=RenderIR[locked]" : "  preview=RenderIR") : "") +
                   "  " + state.status;
  draw_text(hdc, 6, height_ - status_h + 5, st, RGB(32,32,32), RGB(240,240,240), false);

  if (!state.status.empty()) {
    const int toast_w = std::min(width_ - 20, static_cast<int>(state.status.size()) * char_w_ + 24);
    const int toast_x = std::max(10, width_ - toast_w - 12);
    fill_rect(hdc, toast_x, menu_h_ + 8, toast_w, line_h_ + 8, RGB(255,248,220));
    draw_text(hdc, toast_x + 10, menu_h_ + 12, state.status, RGB(32,32,32), RGB(255,248,220), false);
  }

  SelectObject(hdc, old_font);
}

} // namespace mtx
