#include "mtx/viewport_manager.hpp"
#include <algorithm>

namespace mtx {

Viewport ViewportManager::visible(const EditorState& state, int pixel_w, int pixel_h, int char_w, int line_h) const {
  const int gutter = 8;
  Viewport v;
  v.first_row = std::max(0, state.scroll_row);
  v.rows = std::max(1, (pixel_h - 32) / std::max(1, line_h));
  v.first_col = std::max(0, state.scroll_col);
  v.cols = std::max(1, (pixel_w - gutter) / std::max(1, char_w));
  return v;
}

void ViewportManager::ensure_cursor_visible(EditorState& state, const std::string& text, int pixel_w, int pixel_h, int char_w, int line_h) const {
  const LineIndex idx = build_line_index(text);
  const RowCol rc = byte_to_row_col(idx, state.cursor_byte);
  const Viewport v = visible(state, pixel_w, pixel_h, char_w, line_h);

  if (rc.row < v.first_row) state.scroll_row = rc.row;
  else if (rc.row >= v.first_row + v.rows) state.scroll_row = rc.row - v.rows + 1;

  if (rc.col < v.first_col) state.scroll_col = rc.col;
  else if (rc.col >= v.first_col + v.cols) state.scroll_col = rc.col - v.cols + 1;

  if (state.scroll_row < 0) state.scroll_row = 0;
  if (state.scroll_col < 0) state.scroll_col = 0;
}

void ViewportManager::scroll_lines(EditorState& state, int delta, const std::string& text) const {
  const LineIndex idx = build_line_index(text);
  const int max_row = std::max(0, static_cast<int>(idx.starts.size()) - 1);
  state.scroll_row = std::max(0, std::min(max_row, state.scroll_row + delta));
}

} // namespace mtx
