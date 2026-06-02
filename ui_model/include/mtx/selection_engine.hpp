#pragma once
#include "mtx/editor_state.hpp"
#include <cstddef>

namespace mtx {

class SelectionEngine {
public:
  void begin_drag(EditorState& state, std::size_t byte);
  void update_drag(EditorState& state, std::size_t byte);
  void end_drag(EditorState& state);
  void select_all(EditorState& state, std::size_t size);
  void clear(EditorState& state);
};

} // namespace mtx
