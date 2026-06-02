#pragma once
#include <cstddef>
#include <string>

namespace mtx {

enum class EditorMode { Insert, Command };

struct Selection {
  bool active{false};
  std::size_t anchor{0};
  std::size_t focus{0};

  std::size_t begin() const { return anchor < focus ? anchor : focus; }
  std::size_t end() const { return anchor < focus ? focus : anchor; }
  bool contains(std::size_t byte) const { return active && byte >= begin() && byte < end(); }
};

struct EditorState {
  EditorMode mode{EditorMode::Insert};
  std::size_t cursor_byte{0};
  int preferred_col{0};
  int scroll_row{0};
  int scroll_col{0};
  Selection selection;
  bool dirty{false};
  std::string status{"loaded"};
};

} // namespace mtx
