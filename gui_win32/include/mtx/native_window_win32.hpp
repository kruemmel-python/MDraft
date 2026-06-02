#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "mtx/editor_runtime.hpp"
#include "mtx/command_registry.hpp"
#include "mtx/input_dispatcher.hpp"
#include "mtx/cursor_controller.hpp"
#include "mtx/viewport_manager.hpp"
#include "mtx/highlight_processor.hpp"
#include "mtx/glyph_metrics_table.hpp"
#include "mtx/selection_engine.hpp"
#include "mtx/cursor_manager.hpp"
#include "mtx/native_menu_win32.hpp"
#include "mtx/html.hpp"
#include "mtx/workspace.hpp"
#include <windows.h>
#include <windowsx.h>
#include <string>
#include <cstdint>

namespace mtx {

class NativeWindowWin32 {
public:
  NativeWindowWin32(int w, int h);
  ~NativeWindowWin32();

  NativeWindowWin32(const NativeWindowWin32&) = delete;
  NativeWindowWin32& operator=(const NativeWindowWin32&) = delete;

  bool run(EditorRuntime& runtime);

private:
  HWND hwnd_{nullptr};
  HFONT font_{nullptr};
  int width_{1100};
  int height_{720};
  int char_w_{8};
  int line_h_{18};
  int baseline_{14};
  int menu_h_{0};
  bool running_{true};
  bool mouse_selecting_{false};
  bool redraw_pending_{false};
  bool preview_visible_{false};
  bool preview_focused_{false};
  bool preview_locked_{false};
  std::string locked_preview_path_;
  std::string locked_preview_markdown_;
  int preview_scroll_y_{0};
  int preview_content_h_{0};
  HtmlTheme preview_theme_{HtmlTheme::Horror};
  WorkspaceIndex workspace_index_;

  EditorRuntime* runtime_{nullptr};
  CommandRegistry registry_;
  NativeMenuWin32 menu_;
  InputDispatcher input_;
  CursorController cursor_;
  ViewportManager viewport_;
  HighlightProcessor highlighter_;
  GlyphMetricsTable metrics_;
  SelectionEngine selection_;
  CursorManager cursor_visual_;

  static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
  LRESULT handle(UINT msg, WPARAM wp, LPARAM lp);

  void draw(HDC hdc);
  void draw_editor(HDC hdc, int editor_w);
  void draw_render_preview(HDC hdc, int x0, int w);
  int editor_area_width() const noexcept;
  int preview_area_x() const noexcept;
  bool point_in_preview(int x, int y) const noexcept;
  void clamp_preview_scroll() noexcept;
  void set_preview_theme(HtmlTheme theme);
  void sync_preview_theme_menu();
  void sync_preview_lock_menu();
  void toggle_preview_lock();
  std::string preview_markdown_source() const;
  std::string preview_source_label() const;
  void invalidate();
  void repaint_now();
  void execute(CommandID id);
  bool choose_markdown_path_for_open(std::string& out_path);
  bool choose_markdown_path_for_save(std::string& out_path);
  bool choose_workspace_root(std::string& out_path);
  bool confirm_discard_if_dirty();
  bool save_via_dialog();
  std::string current_search_query() const;
  void open_workspace(const std::string& path);
  void reindex_workspace();
  void search_workspace_from_editor();
  void navigate_symbol_from_editor();
  void validate_links_from_workspace();
  void suggest_images_from_editor();
  void run_diagnostics_from_workspace();
  void show_git_status_light();
  void insert_snippet_from_editor();
  void insert_image_from_dialog();
  bool choose_image_path_for_open(std::string& out_path);
  bool open_workspace_symbol(const MarkdownSymbol& symbol);
  void handle_key(WPARAM vk, LPARAM lp);
  void handle_char(WPARAM ch);
  void handle_mouse_down(int x, int y, bool right);
  void handle_mouse_move(int x, int y, WPARAM flags);
  void handle_mouse_up();
  void handle_wheel(short delta, int x, int y);
  std::size_t mouse_to_byte(int x, int y, const std::string& text, const EditorState& state) const;

  void fill_rect(HDC hdc, int x, int y, int w, int h, COLORREF color);
  void draw_text(HDC hdc, int x, int y, const std::string& s, COLORREF fg, COLORREF bg, bool fill_bg=false);
};

} // namespace mtx
