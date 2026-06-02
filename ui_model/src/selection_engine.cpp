#include "mtx/selection_engine.hpp"

namespace mtx {

void SelectionEngine::begin_drag(EditorState& state, std::size_t byte) {
  state.cursor_byte = byte;
  state.selection.active = true;
  state.selection.anchor = byte;
  state.selection.focus = byte;
}

void SelectionEngine::update_drag(EditorState& state, std::size_t byte) {
  state.cursor_byte = byte;
  if (!state.selection.active) {
    state.selection.active = true;
    state.selection.anchor = byte;
  }
  state.selection.focus = byte;
}

void SelectionEngine::end_drag(EditorState& state) {
  if (state.selection.active && state.selection.anchor == state.selection.focus) state.selection.active = false;
}

void SelectionEngine::select_all(EditorState& state, std::size_t size) {
  state.selection.active = size > 0;
  state.selection.anchor = 0;
  state.selection.focus = size;
  state.cursor_byte = size;
}

void SelectionEngine::clear(EditorState& state) { state.selection.active = false; }

} // namespace mtx
