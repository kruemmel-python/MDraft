#pragma once
#include "mtx/editor_state.hpp"
#include <cstddef>
#include <string>
#include <vector>

namespace mtx {

struct LineIndex {
  std::vector<std::size_t> starts;
  std::vector<std::size_t> ends;
};

struct RowCol {
  int row{0};
  int col{0};
};

LineIndex build_line_index(const std::string& text);
RowCol byte_to_row_col(const LineIndex& idx, std::size_t byte);
std::size_t row_col_to_byte(const LineIndex& idx, int row, int col);

class CursorController {
public:
  void sync(EditorState& state, const std::string& text) const;
  void move_left(EditorState& state, const std::string& text, bool select) const;
  void move_right(EditorState& state, const std::string& text, bool select) const;
  void move_up(EditorState& state, const std::string& text, bool select) const;
  void move_down(EditorState& state, const std::string& text, bool select) const;
  void begin_selection(EditorState& state) const;
  void clear_selection(EditorState& state) const;
};

} // namespace mtx
