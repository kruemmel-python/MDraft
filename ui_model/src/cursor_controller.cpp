#include "mtx/cursor_controller.hpp"
#include <algorithm>

namespace mtx {

LineIndex build_line_index(const std::string& text) {
  LineIndex idx;
  idx.starts.push_back(0);
  for (std::size_t i = 0; i < text.size(); ++i) {
    if (text[i] == '\n') {
      idx.ends.push_back(i);
      idx.starts.push_back(i + 1);
    }
  }
  idx.ends.push_back(text.size());
  return idx;
}

RowCol byte_to_row_col(const LineIndex& idx, std::size_t byte) {
  if (idx.starts.empty()) return {};
  auto it = std::upper_bound(idx.starts.begin(), idx.starts.end(), byte);
  int row = static_cast<int>((it == idx.starts.begin()) ? 0 : (it - idx.starts.begin() - 1));
  if (row < 0) row = 0;
  if (row >= static_cast<int>(idx.starts.size())) row = static_cast<int>(idx.starts.size() - 1);
  const int col = static_cast<int>(byte > idx.starts[static_cast<std::size_t>(row)] ? byte - idx.starts[static_cast<std::size_t>(row)] : 0);
  return {row, col};
}

std::size_t row_col_to_byte(const LineIndex& idx, int row, int col) {
  if (idx.starts.empty()) return 0;
  row = std::max(0, std::min(row, static_cast<int>(idx.starts.size()) - 1));
  const std::size_t start = idx.starts[static_cast<std::size_t>(row)];
  const std::size_t end = idx.ends[static_cast<std::size_t>(row)];
  const std::size_t want = start + static_cast<std::size_t>(std::max(0, col));
  return std::min(want, end);
}

void CursorController::sync(EditorState& state, const std::string& text) const {
  if (state.cursor_byte > text.size()) state.cursor_byte = text.size();
  if (state.selection.active) {
    if (state.selection.anchor > text.size()) state.selection.anchor = text.size();
    if (state.selection.focus > text.size()) state.selection.focus = text.size();
    if (state.selection.anchor == state.selection.focus) state.selection.active = false;
  }
}

void CursorController::begin_selection(EditorState& state) const {
  if (!state.selection.active) {
    state.selection.active = true;
    state.selection.anchor = state.cursor_byte;
    state.selection.focus = state.cursor_byte;
  }
}
void CursorController::clear_selection(EditorState& state) const { state.selection.active = false; }

void CursorController::move_left(EditorState& state, const std::string& text, bool select) const {
  if (select) begin_selection(state); else clear_selection(state);
  if (state.cursor_byte > 0) --state.cursor_byte;
  state.selection.focus = state.cursor_byte;
  state.preferred_col = byte_to_row_col(build_line_index(text), state.cursor_byte).col;
}

void CursorController::move_right(EditorState& state, const std::string& text, bool select) const {
  if (select) begin_selection(state); else clear_selection(state);
  if (state.cursor_byte < text.size()) ++state.cursor_byte;
  state.selection.focus = state.cursor_byte;
  state.preferred_col = byte_to_row_col(build_line_index(text), state.cursor_byte).col;
}

void CursorController::move_up(EditorState& state, const std::string& text, bool select) const {
  if (select) begin_selection(state); else clear_selection(state);
  LineIndex idx = build_line_index(text);
  RowCol rc = byte_to_row_col(idx, state.cursor_byte);
  if (rc.row > 0) {
    state.cursor_byte = row_col_to_byte(idx, rc.row - 1, state.preferred_col);
  }
  state.selection.focus = state.cursor_byte;
}

void CursorController::move_down(EditorState& state, const std::string& text, bool select) const {
  if (select) begin_selection(state); else clear_selection(state);
  LineIndex idx = build_line_index(text);
  RowCol rc = byte_to_row_col(idx, state.cursor_byte);
  if (rc.row + 1 < static_cast<int>(idx.starts.size())) {
    state.cursor_byte = row_col_to_byte(idx, rc.row + 1, state.preferred_col);
  }
  state.selection.focus = state.cursor_byte;
}

} // namespace mtx
