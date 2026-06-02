#include "mtx/command_dispatcher.hpp"

namespace mtx {

static bool erase_selection_if_any(CommandManager& commands, GapBuffer& buffer, EditorState& state) {
  if (state.selection.active && state.selection.begin() != state.selection.end()) {
    EditResult r = commands.erase_range(buffer, state.selection.begin(), state.selection.end());
    state.cursor_byte = r.cursor;
    state.selection.active = false;
    state.dirty = state.dirty || r.changed;
    return r.changed;
  }
  return false;
}

static void apply_result(EditorState& state, const EditResult& r) {
  if (!r.changed) return;
  state.cursor_byte = r.cursor;
  state.selection.active = false;
  state.dirty = true;
}

void CommandDispatcher::insert_text(CommandManager& commands, GapBuffer& buffer, EditorState& state, const std::string& text) const {
  erase_selection_if_any(commands, buffer, state);
  apply_result(state, commands.insert(buffer, state.cursor_byte, text));
}

void CommandDispatcher::smart_newline(CommandManager& commands, GapBuffer& buffer, EditorState& state) const {
  erase_selection_if_any(commands, buffer, state);
  apply_result(state, commands.smart_newline(buffer, state.cursor_byte));
}

void CommandDispatcher::backspace(CommandManager& commands, GapBuffer& buffer, EditorState& state) const {
  if (erase_selection_if_any(commands, buffer, state)) return;
  apply_result(state, commands.backspace(buffer, state.cursor_byte));
}

void CommandDispatcher::delete_at(CommandManager& commands, GapBuffer& buffer, EditorState& state) const {
  if (erase_selection_if_any(commands, buffer, state)) return;
  apply_result(state, commands.delete_at(buffer, state.cursor_byte));
}

void CommandDispatcher::undo(CommandManager& commands, GapBuffer& buffer, EditorState& state) const {
  EditResult r = commands.undo(buffer, state.cursor_byte);
  if (r.changed) {
    state.cursor_byte = r.cursor;
    state.selection.active = false;
    state.dirty = true;
    state.status = "undo";
  }
}

void CommandDispatcher::redo(CommandManager& commands, GapBuffer& buffer, EditorState& state) const {
  EditResult r = commands.redo(buffer, state.cursor_byte);
  if (r.changed) {
    state.cursor_byte = r.cursor;
    state.selection.active = false;
    state.dirty = true;
    state.status = "redo";
  }
}

} // namespace mtx
