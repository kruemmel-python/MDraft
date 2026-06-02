#pragma once
#include "mtx/cursor_controller.hpp"
#include "mtx/glyph_metrics_table.hpp"
#include "mtx/input_dispatcher.hpp"
#include "mtx/highlight_processor.hpp"
#include "mtx/viewport_manager.hpp"
#include "mtx/command_registry.hpp"
#include "mtx/menu_bar.hpp"
#include "mtx/selection_engine.hpp"
#include "mtx/cursor_manager.hpp"
#include "mtx/context_projection.hpp"
#include "mtx/editor_runtime.hpp"
#include <string>
#include <X11/Xlib.h>

namespace mtx {

class NativeWindowX11 {
public:
  NativeWindowX11(int w, int h);
  ~NativeWindowX11();

  NativeWindowX11(const NativeWindowX11&) = delete;
  NativeWindowX11& operator=(const NativeWindowX11&) = delete;

  bool run(EditorRuntime& runtime);
  void draw(const GapBuffer& buffer, const EditorState& state);

private:
  Display* d_{nullptr};
  int screen_{0};
  Window win_{0};
  GC gc_{0};
  Atom wm_delete_{0};
  int width_{1000};
  int height_{700};
  int char_w_{8};
  int line_h_{16};
  int baseline_{12};
  GlyphMetricsTable metrics_;
  CommandRegistry registry_;
  MenuBar menu_;
  bool mouse_selecting_{false};
  bool redraw_pending_{true};

  CursorController cursor_;
  CursorManager cursor_visual_;
  ViewportManager viewport_;
  SelectionEngine selection_;
  ContextProjection context_;
  HighlightProcessor highlighter_;
  InputDispatcher input_;

  void set_color(unsigned long rgb);
  void draw_text_cell(int x, int y, char c, TextStyle s);
  void draw_menu();
  bool execute_command(CommandID id, EditorRuntime& runtime, bool& running);
  void fill_rect(int x, int y, int w, int h, unsigned long rgb);
  void handle_key(XKeyEvent& ev, EditorRuntime& runtime, bool& running);
  void handle_button(XButtonEvent& ev, EditorRuntime& runtime);
  void handle_motion(XMotionEvent& ev, EditorRuntime& runtime);
  std::size_t mouse_to_byte(int x, int y, const std::string& text, const EditorState& state) const;
};

} // namespace mtx
