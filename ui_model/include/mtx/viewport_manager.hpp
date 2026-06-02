#pragma once
#include "mtx/cursor_controller.hpp"
#include "mtx/editor_state.hpp"

namespace mtx {

struct Viewport {
  int first_row{0};
  int rows{1};
  int first_col{0};
  int cols{1};
};

class ViewportManager {
public:
  Viewport visible(const EditorState& state, int pixel_w, int pixel_h, int char_w, int line_h) const;
  void ensure_cursor_visible(EditorState& state, const std::string& text, int pixel_w, int pixel_h, int char_w, int line_h) const;
  void scroll_lines(EditorState& state, int delta, const std::string& text) const;
};

} // namespace mtx
