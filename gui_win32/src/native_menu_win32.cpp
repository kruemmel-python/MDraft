#include "mtx/native_menu_win32.hpp"

#include <string>

namespace mtx {
namespace {

std::wstring widen_ascii(const std::string& s) {
  std::wstring out;
  out.reserve(s.size());
  for (unsigned char c : s) {
    out.push_back(static_cast<wchar_t>(c));
  }
  return out;
}

std::wstring label_with_shortcut(const wchar_t* label,
                                 const CommandRegistry& registry,
                                 CommandID id) {
  std::wstring out(label);
  const std::string shortcut = registry.shortcut_label(id);
  if (!shortcut.empty()) {
    out += L"\t";
    out += widen_ascii(shortcut);
  }
  return out;
}

} // namespace

NativeMenuWin32::~NativeMenuWin32() {
  if (root_ != nullptr) {
    DestroyMenu(root_);
    root_ = nullptr;
  }
}

UINT NativeMenuWin32::win_id(CommandID id) noexcept {
  switch (id) {
    case CommandID::NewFile: return 1000u;
    case CommandID::OpenFile: return 1009u;
    case CommandID::Save: return 1001u;
    case CommandID::SaveAs: return 1008u;
    case CommandID::ExportHtml: return 1002u;
    case CommandID::ExportHtmlGitHub: return 10010u;
    case CommandID::ExportHtmlCyberpunk: return 1004u;
    case CommandID::ExportHtmlDystopia: return 1005u;
    case CommandID::ExportHtmlHorror: return 1006u;
    case CommandID::ExportHtmlAdventure: return 1007u;
    case CommandID::PreviewThemeStandard: return 1210u;
    case CommandID::PreviewThemeGitHub: return 1215u;
    case CommandID::PreviewThemeCyberpunk: return 1211u;
    case CommandID::PreviewThemeDystopia: return 1212u;
    case CommandID::PreviewThemeHorror: return 1213u;
    case CommandID::PreviewThemeAdventure: return 1214u;
    case CommandID::Quit: return 1003u;
    case CommandID::Undo: return 1101u;
    case CommandID::Redo: return 1102u;
    case CommandID::Copy: return 1103u;
    case CommandID::Cut: return 1104u;
    case CommandID::Paste: return 1105u;
    case CommandID::SelectAll: return 1106u;
    case CommandID::ToggleMode: return 1107u;
    case CommandID::TogglePreview: return 1201u;
    case CommandID::TogglePreviewLock: return 1202u;
    case CommandID::OpenWorkspace: return 1301u;
    case CommandID::ReindexWorkspace: return 1302u;
    case CommandID::WorkspaceStatus: return 1303u;
    case CommandID::WorkspaceSearch: return 1304u;
    case CommandID::WorkspaceSymbols: return 1305u;
    case CommandID::ValidateLinks: return 1306u;
    case CommandID::ImagePathSuggestions: return 1307u;
    case CommandID::RunDiagnostics: return 1308u;
    case CommandID::GitStatus: return 1309u;
    case CommandID::InsertSnippet: return 1110u;
    case CommandID::InsertImage: return 1111u;
    case CommandID::About: return 1901u;
    default: return 0u;
  }
}

CommandID NativeMenuWin32::command_id(UINT id) noexcept {
  switch (id) {
    case 1000u: return CommandID::NewFile;
    case 1009u: return CommandID::OpenFile;
    case 1001u: return CommandID::Save;
    case 1008u: return CommandID::SaveAs;
    case 1002u: return CommandID::ExportHtml;
    case 10010u: return CommandID::ExportHtmlGitHub;
    case 1004u: return CommandID::ExportHtmlCyberpunk;
    case 1005u: return CommandID::ExportHtmlDystopia;
    case 1006u: return CommandID::ExportHtmlHorror;
    case 1007u: return CommandID::ExportHtmlAdventure;
    case 1210u: return CommandID::PreviewThemeStandard;
    case 1215u: return CommandID::PreviewThemeGitHub;
    case 1211u: return CommandID::PreviewThemeCyberpunk;
    case 1212u: return CommandID::PreviewThemeDystopia;
    case 1213u: return CommandID::PreviewThemeHorror;
    case 1214u: return CommandID::PreviewThemeAdventure;
    case 1003u: return CommandID::Quit;
    case 1101u: return CommandID::Undo;
    case 1102u: return CommandID::Redo;
    case 1103u: return CommandID::Copy;
    case 1104u: return CommandID::Cut;
    case 1105u: return CommandID::Paste;
    case 1106u: return CommandID::SelectAll;
    case 1107u: return CommandID::ToggleMode;
    case 1201u: return CommandID::TogglePreview;
    case 1202u: return CommandID::TogglePreviewLock;
    case 1301u: return CommandID::OpenWorkspace;
    case 1302u: return CommandID::ReindexWorkspace;
    case 1303u: return CommandID::WorkspaceStatus;
    case 1304u: return CommandID::WorkspaceSearch;
    case 1305u: return CommandID::WorkspaceSymbols;
    case 1306u: return CommandID::ValidateLinks;
    case 1307u: return CommandID::ImagePathSuggestions;
    case 1308u: return CommandID::RunDiagnostics;
    case 1309u: return CommandID::GitStatus;
    case 1110u: return CommandID::InsertSnippet;
    case 1111u: return CommandID::InsertImage;
    case 1901u: return CommandID::About;
    default: return CommandID::NoCommand;
  }
}

void NativeMenuWin32::append_separator(HMENU menu) {
  AppendMenuW(menu, MF_SEPARATOR, 0u, nullptr);
}

void NativeMenuWin32::append_command(HMENU menu,
                                     const CommandRegistry& registry,
                                     CommandID id,
                                     const wchar_t* label) {
  const std::wstring text = label_with_shortcut(label, registry, id);
  AppendMenuW(menu, MF_STRING, static_cast<UINT_PTR>(win_id(id)), text.c_str());
}

void NativeMenuWin32::attach(HWND hwnd, const CommandRegistry& registry) {
  hwnd_ = hwnd;
  if (root_ != nullptr) {
    DestroyMenu(root_);
    root_ = nullptr;
  }

  root_ = CreateMenu();
  HMENU file = CreatePopupMenu();
  HMENU edit = CreatePopupMenu();
  HMENU view = CreatePopupMenu();
  HMENU project = CreatePopupMenu();
  HMENU help = CreatePopupMenu();

  append_command(file, registry, CommandID::NewFile, L"&Neu");
  append_command(file, registry, CommandID::OpenFile, L"Ö&ffnen...");
  append_separator(file);
  HMENU html = CreatePopupMenu();
  append_command(html, registry, CommandID::ExportHtml, L"&Standard");
  append_command(html, registry, CommandID::ExportHtmlGitHub, L"&GitHub README-Stil");
  append_command(html, registry, CommandID::ExportHtmlCyberpunk, L"&Cyberpunk-Stil");
  append_command(html, registry, CommandID::ExportHtmlDystopia, L"&Dystopie-Stil");
  append_command(html, registry, CommandID::ExportHtmlHorror, L"&Horror-Stil");
  append_command(html, registry, CommandID::ExportHtmlAdventure, L"&Abenteuer-/Spannung-Stil");

  append_command(file, registry, CommandID::Save, L"&Speichern");
  append_command(file, registry, CommandID::SaveAs, L"Speichern &unter...");
  AppendMenuW(file, MF_POPUP, reinterpret_cast<UINT_PTR>(html), L"Als &HTML exportieren");
  append_separator(file);
  append_command(file, registry, CommandID::Quit, L"&Beenden");

  append_command(edit, registry, CommandID::Undo, L"&Rückgängig");
  append_command(edit, registry, CommandID::Redo, L"&Wiederholen");
  append_separator(edit);
  append_command(edit, registry, CommandID::Copy, L"&Kopieren");
  append_command(edit, registry, CommandID::Cut, L"&Ausschneiden");
  append_command(edit, registry, CommandID::Paste, L"&Einfügen");
  append_separator(edit);
  append_command(edit, registry, CommandID::InsertSnippet, L"Snippet ein&fügen");
  append_command(edit, registry, CommandID::InsertImage, L"&Bild einfügen...");
  append_separator(edit);
  append_command(edit, registry, CommandID::SelectAll, L"Alles &markieren");

  HMENU preview_theme = CreatePopupMenu();
  append_command(preview_theme, registry, CommandID::PreviewThemeStandard, L"&Standard");
  append_command(preview_theme, registry, CommandID::PreviewThemeGitHub, L"&GitHub");
  append_command(preview_theme, registry, CommandID::PreviewThemeCyberpunk, L"&Cyberpunk");
  append_command(preview_theme, registry, CommandID::PreviewThemeDystopia, L"&Dystopie");
  append_command(preview_theme, registry, CommandID::PreviewThemeHorror, L"&Horror");
  append_command(preview_theme, registry, CommandID::PreviewThemeAdventure, L"&Abenteuer");

  append_command(view, registry, CommandID::ToggleMode, L"Insert/Command-&Modus wechseln");
  append_separator(view);
  append_command(view, registry, CommandID::TogglePreview, L"&RenderIR/HTML Live-Vorschau anzeigen");
  append_command(view, registry, CommandID::TogglePreviewLock, L"Preview &sperren");
  AppendMenuW(view, MF_POPUP, reinterpret_cast<UINT_PTR>(preview_theme), L"Preview-&Thema");

  append_command(project, registry, CommandID::OpenWorkspace, L"Workspace &öffnen...");
  append_command(project, registry, CommandID::ReindexWorkspace, L"Workspace &neu indizieren");
  append_separator(project);
  append_command(project, registry, CommandID::WorkspaceSearch, L"Workspace &suchen...");
  append_command(project, registry, CommandID::WorkspaceSymbols, L"Symbol-&Navigation...");
  append_command(project, registry, CommandID::ValidateLinks, L"&Links validieren...");
  append_command(project, registry, CommandID::ImagePathSuggestions, L"&Bildpfad-Vorschläge...");
  append_command(project, registry, CommandID::RunDiagnostics, L"&Diagnostics/Linting...");
  append_command(project, registry, CommandID::GitStatus, L"&Git-Status light...");
  append_command(project, registry, CommandID::WorkspaceStatus, L"Workspace-&Status");

  append_command(help, registry, CommandID::About, L"&Über MDraft");

  AppendMenuW(root_, MF_POPUP, reinterpret_cast<UINT_PTR>(file), L"&Datei");
  AppendMenuW(root_, MF_POPUP, reinterpret_cast<UINT_PTR>(edit), L"&Bearbeiten");
  AppendMenuW(root_, MF_POPUP, reinterpret_cast<UINT_PTR>(view), L"&Ansicht");
  AppendMenuW(root_, MF_POPUP, reinterpret_cast<UINT_PTR>(project), L"&Projekt");
  AppendMenuW(root_, MF_POPUP, reinterpret_cast<UINT_PTR>(help), L"&Hilfe");

  SetMenu(hwnd_, root_);
  DrawMenuBar(hwnd_);
}

CommandID NativeMenuWin32::command_from_wparam(WPARAM wp) const noexcept {
  return command_id(LOWORD(wp));
}

void NativeMenuWin32::enable(CommandID id, bool enabled) noexcept {
  if (root_ == nullptr) {
    return;
  }
  const UINT flags = MF_BYCOMMAND | (enabled ? MF_ENABLED : (MF_DISABLED | MF_GRAYED));
  EnableMenuItem(root_, win_id(id), flags);
}


void NativeMenuWin32::set_checked(CommandID id, bool checked) noexcept {
  if (root_ == nullptr) return;
  CheckMenuItem(root_, win_id(id), MF_BYCOMMAND | (checked ? MF_CHECKED : MF_UNCHECKED));
  DrawMenuBar(hwnd_);
}

void NativeMenuWin32::sync_enabled(const EditorRuntime& runtime) noexcept {
  const EditorState& state = runtime.state();
  const bool has_selection = state.selection.active && state.selection.begin() != state.selection.end();
  enable(CommandID::NewFile, true);
  enable(CommandID::OpenFile, true);
  enable(CommandID::Save, true);
  enable(CommandID::SaveAs, true);
  enable(CommandID::Undo, runtime.can_undo());
  enable(CommandID::Redo, runtime.can_redo());
  enable(CommandID::Copy, has_selection);
  enable(CommandID::Cut, has_selection);
  enable(CommandID::Paste, true);
  enable(CommandID::TogglePreview, true);
  enable(CommandID::TogglePreviewLock, true);
  enable(CommandID::OpenWorkspace, true);
  enable(CommandID::ReindexWorkspace, true);
  enable(CommandID::WorkspaceStatus, true);
  enable(CommandID::WorkspaceSearch, true);
  enable(CommandID::WorkspaceSymbols, true);
  enable(CommandID::ValidateLinks, true);
  enable(CommandID::ImagePathSuggestions, true);
  enable(CommandID::RunDiagnostics, true);
  DrawMenuBar(hwnd_);
}

} // namespace mtx
