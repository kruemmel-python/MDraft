#include "mtx/native_window_x11.hpp"
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <unistd.h>

namespace mtx {

static unsigned long rgb(Display* d, int screen, unsigned long v) {
  (void)d; (void)screen;
  return v;
}

NativeWindowX11::NativeWindowX11(int w, int h) : width_(w), height_(h), menu_(28), input_(&registry_) {
  d_ = XOpenDisplay(nullptr);
  if (!d_) throw std::runtime_error("XOpenDisplay failed");
  screen_ = DefaultScreen(d_);
  win_ = XCreateSimpleWindow(d_, RootWindow(d_, screen_), 50, 50, width_, height_, 1,
                             BlackPixel(d_, screen_), WhitePixel(d_, screen_));
  XStoreName(d_, win_, "MDraft X11 - v0.6.2 feedback/html kernel");
  XSelectInput(d_, win_, ExposureMask | KeyPressMask | StructureNotifyMask |
                        ButtonPressMask | ButtonReleaseMask | PointerMotionMask);
  wm_delete_ = XInternAtom(d_, "WM_DELETE_WINDOW", False);
  XSetWMProtocols(d_, win_, &wm_delete_, 1);
  gc_ = XCreateGC(d_, win_, 0, nullptr);
  XFontStruct* fixed = XLoadQueryFont(d_, "fixed");
  if (fixed) {
    XSetFont(d_, gc_, fixed->fid);
    const int cw = XTextWidth(fixed, "M", 1);
    char_w_ = cw > 0 ? cw : char_w_;
    line_h_ = (fixed->ascent + fixed->descent) > 0 ? fixed->ascent + fixed->descent + 2 : line_h_;
    baseline_ = fixed->ascent + 1;
    metrics_.set_fixed_width(char_w_);
    metrics_.set_vertical_metrics(line_h_, baseline_);
  }
  menu_.rebuild(registry_, char_w_);
  XMapWindow(d_, win_);
}

NativeWindowX11::~NativeWindowX11() {
  if (d_) {
    if (gc_) XFreeGC(d_, gc_);
    if (win_) XDestroyWindow(d_, win_);
    XCloseDisplay(d_);
  }
}

void NativeWindowX11::set_color(unsigned long v) { XSetForeground(d_, gc_, rgb(d_, screen_, v)); }

void NativeWindowX11::fill_rect(int x, int y, int w, int h, unsigned long v) {
  set_color(v);
  XFillRectangle(d_, win_, gc_, x, y, static_cast<unsigned int>(std::max(0,w)), static_cast<unsigned int>(std::max(0,h)));
}

void NativeWindowX11::draw_text_cell(int x, int y, char c, TextStyle s) {
  const int cw = metrics_.width(static_cast<unsigned char>(c ? c : ' '));
  if (has_style(s, TextStyle::Selection)) fill_rect(x, y, cw, line_h_, 0xcfe8ff);
  else if (has_style(s, TextStyle::Heading1) || has_style(s, TextStyle::Heading2) || has_style(s, TextStyle::Heading3)) fill_rect(x, y, cw, line_h_, 0xf2f2f2);
  else if (has_style(s, TextStyle::Code)) fill_rect(x, y, cw, line_h_, 0xf7f1e3);

  unsigned long fg = 0x202020;
  if (has_style(s, TextStyle::Selection)) fg = 0x000000;
  else if (has_style(s, TextStyle::Heading1) || has_style(s, TextStyle::Heading2) || has_style(s, TextStyle::Heading3)) fg = 0x000000;
  else if (has_style(s, TextStyle::Quote)) fg = 0x008000;
  else if (has_style(s, TextStyle::Math)) fg = 0x6b238e;
  else if (has_style(s, TextStyle::Code)) fg = 0x9b3a00;
  else if (has_style(s, TextStyle::List)) fg = 0x003399;

  set_color(fg);
  char out[2] = {c ? c : ' ', 0};
  XDrawString(d_, win_, gc_, x, y + baseline_, out, 1);

  if (has_style(s, TextStyle::Bold)) XDrawString(d_, win_, gc_, x + 1, y + baseline_, out, 1);
  if (has_style(s, TextStyle::Italic)) XDrawLine(d_, win_, gc_, x + 1, y + line_h_ - 2, x + cw - 2, y + line_h_ - 2);
}

void NativeWindowX11::draw_menu() {
  fill_rect(0, 0, width_, menu_.height(), 0xe9e9e9);
  set_color(0xb0b0b0);
  XDrawLine(d_, win_, gc_, 0, menu_.height() - 1, width_, menu_.height() - 1);
  for (const auto& c : menu_.cells()) {
    fill_rect(c.x, c.y + 3, c.w, c.h - 6, 0xf7f7f7);
    set_color(0x202020);
    std::string label = c.label;
    if (!c.shortcut.empty()) label += "  " + c.shortcut;
    XDrawString(d_, win_, gc_, c.x + 6, c.y + baseline_ + 5, label.c_str(), static_cast<int>(label.size()));
  }
}

void NativeWindowX11::draw(const GapBuffer& buffer, const EditorState& state) {
  const std::string text = buffer.str();
  const LineIndex idx = build_line_index(text);
  const AttributeBuffer attrs = highlighter_.build(text, state.selection);

  fill_rect(0, 0, width_, height_, 0xffffff);
  draw_menu();

  const int status_h = 22;
  const int top = menu_.height() + 6;
  const int left = 8;
  const int editor_h = height_ - top - status_h - 8;
  const Viewport v = viewport_.visible(state, width_ - 36, editor_h, char_w_, line_h_);
  const int text_x = left + 48;
  const int text_y = top;

  for (int vr = 0; vr < v.rows; ++vr) {
    const int row = v.first_row + vr;
    if (row >= static_cast<int>(idx.starts.size())) break;
    const std::size_t e = idx.ends[static_cast<std::size_t>(row)];
    const int y = text_y + vr * line_h_;

    char rn[16];
    std::snprintf(rn, sizeof(rn), "%4d", row + 1);
    set_color(0x909090);
    XDrawString(d_, win_, gc_, left, y + baseline_, rn, static_cast<int>(std::strlen(rn)));

    std::size_t bi = row_col_to_byte(idx, row, v.first_col);
    int x = text_x;
    while (bi < e && x < width_ - 24) {
      char c = text[bi];
      TextStyle s = attrs.at(bi);
      const char draw_c = (c == '\t') ? ' ' : c;
      draw_text_cell(x, y, draw_c, s);
      x += metrics_.width(static_cast<unsigned char>(draw_c));
      ++bi;
    }
  }

  CursorVisual cv = cursor_visual_.visual_for_byte(text, state, v, metrics_, text_x, text_y);
  if (cv.visible) fill_rect(cv.x, cv.y, 2, cv.h, state.mode == EditorMode::Insert ? 0x000000 : 0xb00000);

  // Scroll bar projection: model-owned state, screen-owned affordance.
  const int total_rows = static_cast<int>(idx.starts.size());
  const int bar_x = width_ - 14;
  const int bar_y = top;
  const int bar_h = std::max(1, editor_h);
  fill_rect(bar_x, bar_y, 8, bar_h, 0xf0f0f0);
  const int thumb_h = std::max(18, (bar_h * v.rows) / std::max(1, total_rows));
  const int thumb_y = bar_y + ((bar_h - thumb_h) * std::max(0, state.scroll_row)) / std::max(1, total_rows - v.rows);
  fill_rect(bar_x, thumb_y, 8, thumb_h, 0x999999);

  if (!state.status.empty() && state.status != "loaded") {
    const int w = std::min(width_ - 24, static_cast<int>(state.status.size()) * char_w_ + 24);
    fill_rect(width_ - w - 12, menu_.height() + 10, w, 26, 0xfff3cd);
    set_color(0x7a4d00);
    XDrawRectangle(d_, win_, gc_, width_ - w - 12, menu_.height() + 10, static_cast<unsigned int>(w), 26);
    XDrawString(d_, win_, gc_, width_ - w, menu_.height() + 10 + baseline_ + 5, state.status.c_str(), static_cast<int>(state.status.size()));
  }

  fill_rect(0, height_ - status_h, width_, status_h, 0xefefef);
  set_color(0x202020);
  std::string title = "MDraft v0.6.2  mode=" + std::string(state.mode == EditorMode::Insert ? "INSERT" : "COMMAND") +
                      "  " + (state.dirty ? "dirty" : "clean") +
                      "  cursor=" + std::to_string(state.cursor_byte) +
                      "  scroll=" + std::to_string(state.scroll_row) +
                      "  " + state.status;
  XDrawString(d_, win_, gc_, 8, height_ - 7, title.c_str(), static_cast<int>(title.size()));

  XFlush(d_);
}

bool NativeWindowX11::execute_command(CommandID id, EditorRuntime& runtime, bool& running) {
  return runtime.execute(id, running);
}

std::size_t NativeWindowX11::mouse_to_byte(int x, int y, const std::string& text, const EditorState& state) const {
  const int top = menu_.height() + 6;
  const int left = 8 + 48;
  const int editor_h = height_ - top - 30;
  const Viewport v = viewport_.visible(state, width_ - 36, editor_h, char_w_, line_h_);
  const LineIndex idx = build_line_index(text);
  const int row = v.first_row + std::max(0, (y - top) / line_h_);
  const int safe_row = std::max(0, std::min(row, static_cast<int>(idx.starts.size()) - 1));
  const std::size_t line_end = idx.ends[static_cast<std::size_t>(safe_row)];
  const std::size_t visible_begin = row_col_to_byte(idx, safe_row, v.first_col);
  const int px = std::max(0, x - left);
  return metrics_.byte_at_x(text, visible_begin, line_end, px);
}

void NativeWindowX11::handle_button(XButtonEvent& ev, EditorRuntime& runtime) {
  EditorState& state = runtime.state();
  const std::string text = runtime.buffer().str();

  if (ev.button == Button4) {
    viewport_.scroll_lines(state, -3, text);
    state.status = "scroll up";
    return;
  }
  if (ev.button == Button5) {
    viewport_.scroll_lines(state, 3, text);
    state.status = "scroll down";
    return;
  }

  if (ev.button == Button1) {
    const std::size_t b = mouse_to_byte(ev.x, ev.y, text, state);
    selection_.begin_drag(state, b);
    mouse_selecting_ = true;
    state.status = "cursor placed";
    return;
  }

  if (ev.button == Button3) {
    const AttributeBuffer attrs = highlighter_.build(text, state.selection);
    const std::size_t b = mouse_to_byte(ev.x, ev.y, text, state);
    state.status = context_.token_label(text, b, attrs);
    return;
  }
}

void NativeWindowX11::handle_motion(XMotionEvent& ev, EditorRuntime& runtime) {
  if (!mouse_selecting_) return;
  EditorState& state = runtime.state();
  const std::string text = runtime.buffer().str();
  selection_.update_drag(state, mouse_to_byte(ev.x, ev.y, text, state));
  state.status = "selecting";
}

static InputKey x11_to_input_key(KeySym sym, int len) {
  if (sym == XK_Return || sym == XK_KP_Enter) return InputKey::Enter;
  if (sym == XK_BackSpace) return InputKey::Backspace;
  if (sym == XK_Delete) return InputKey::DeleteKey;
  if (sym == XK_Left) return InputKey::Left;
  if (sym == XK_Right) return InputKey::Right;
  if (sym == XK_Up) return InputKey::Up;
  if (sym == XK_Down) return InputKey::Down;
  if (sym == XK_Page_Up) return InputKey::PageUp;
  if (sym == XK_Page_Down) return InputKey::PageDown;
  if (sym == XK_Escape) return InputKey::Escape;
  if (len > 0) return InputKey::Character;
  return InputKey::NoInput;
}

void NativeWindowX11::handle_key(XKeyEvent& ev, EditorRuntime& runtime, bool& running) {
  EditorState& state = runtime.state();
  GapBuffer& buffer = runtime.buffer();
  KeySym sym = 0;
  char buf[32] = {};
  const int len = XLookupString(&ev, buf, sizeof(buf), &sym, nullptr);
  const bool ctrl = (ev.state & ControlMask) != 0;
  const bool shift = (ev.state & ShiftMask) != 0;

  std::string text = buffer.str();
  cursor_.sync(state, text);

  std::string chars;
  InputKey input_key = x11_to_input_key(sym, len);
  if (ctrl && ((sym >= XK_a && sym <= XK_z) || (sym >= XK_A && sym <= XK_Z))) {
    char c = static_cast<char>((sym >= XK_A && sym <= XK_Z) ? (sym - XK_A + 'a') : (sym - XK_a + 'a'));
    chars.assign(1, c);
    input_key = InputKey::Character;
  } else if (len > 0) {
    chars.assign(buf, buf + len);
    bool printable = true;
    for (char c : chars) if (static_cast<unsigned char>(c) < 32) printable = false;
    if (!printable && !ctrl) chars.clear();
  }

  InputEvent iev{input_key, chars, ctrl, shift};
  const EditorAction action = input_.translate(iev, state.mode == EditorMode::Insert);

  switch (action.kind) {
    case ActionKind::Command:
      execute_command(action.command, runtime, running);
      break;
    case ActionKind::MoveLeft:
      cursor_.move_left(state, text, action.select);
      break;
    case ActionKind::MoveRight:
      cursor_.move_right(state, text, action.select);
      break;
    case ActionKind::MoveUp:
      cursor_.move_up(state, text, action.select);
      break;
    case ActionKind::MoveDown:
      cursor_.move_down(state, text, action.select);
      break;
    case ActionKind::PageUp:
      viewport_.scroll_lines(state, -20, text);
      break;
    case ActionKind::PageDown:
      viewport_.scroll_lines(state, 20, text);
      break;
    case ActionKind::Backspace:
      runtime.backspace();
      break;
    case ActionKind::DeleteAt:
      runtime.delete_at();
      break;
    case ActionKind::SmartNewline:
      runtime.smart_newline();
      break;
    case ActionKind::InsertText:
      runtime.insert_text(action.text);
      break;
    case ActionKind::Save:
    case ActionKind::ExportHtml:
    case ActionKind::Quit:
    case ActionKind::Undo:
    case ActionKind::Redo:
    case ActionKind::ToggleMode:
      // Legacy variants are intentionally accepted but no longer generated by InputDispatcher.
      break;
    case ActionKind::NoAction:
      break;
  }

  text = buffer.str();
  cursor_.sync(state, text);
  viewport_.ensure_cursor_visible(state, text, width_ - 36, height_ - menu_.height() - 36, char_w_, line_h_);
}

bool NativeWindowX11::run(EditorRuntime& runtime) {
  bool running = true;
  redraw_pending_ = true;
  while (running) {
    while (XPending(d_) > 0) {
      XEvent ev;
      XNextEvent(d_, &ev);
      if (ev.type == Expose) redraw_pending_ = true;
      else if (ev.type == ConfigureNotify) {
        width_ = ev.xconfigure.width;
        height_ = ev.xconfigure.height;
        menu_.rebuild(registry_, char_w_);
        redraw_pending_ = true;
      } else if (ev.type == ClientMessage) {
        if (static_cast<Atom>(ev.xclient.data.l[0]) == wm_delete_) running = false;
      } else if (ev.type == KeyPress) {
        handle_key(ev.xkey, runtime, running);
        redraw_pending_ = true;
      } else if (ev.type == ButtonPress) {
        bool handled_menu = false;
        if (menu_.contains(ev.xbutton.x, ev.xbutton.y) && ev.xbutton.button == Button1) {
          const CommandID id = menu_.command_at(ev.xbutton.x, ev.xbutton.y);
          if (id != CommandID::NoCommand) {
            handled_menu = execute_command(id, runtime, running);
          }
        }
        if (!handled_menu) handle_button(ev.xbutton, runtime);
        redraw_pending_ = true;
      } else if (ev.type == ButtonRelease) {
        mouse_selecting_ = false;
        selection_.end_drag(runtime.state());
        redraw_pending_ = true;
      } else if (ev.type == MotionNotify) {
        handle_motion(ev.xmotion, runtime);
        redraw_pending_ = true;
      }
    }

    if (redraw_pending_) {
      draw(runtime.buffer(), runtime.state());
      redraw_pending_ = false;
    }
    usleep(8000);
  }
  return true;
}

} // namespace mtx
