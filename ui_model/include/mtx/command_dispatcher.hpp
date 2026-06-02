#pragma once
#include "mtx/command_manager.hpp"
#include "mtx/editor_state.hpp"
#include "mtx/gap_buffer.hpp"
#include <string>

namespace mtx {

enum class CommandResult {
  None, Save, ExportHtml, Quit, Redraw
};

// Mutating dispatcher: UI-level selection/editor-state semantics over core CommandManager.
class CommandDispatcher {
public:
  void insert_text(CommandManager& commands, GapBuffer& buffer, EditorState& state, const std::string& text) const;
  void smart_newline(CommandManager& commands, GapBuffer& buffer, EditorState& state) const;
  void backspace(CommandManager& commands, GapBuffer& buffer, EditorState& state) const;
  void delete_at(CommandManager& commands, GapBuffer& buffer, EditorState& state) const;
  void undo(CommandManager& commands, GapBuffer& buffer, EditorState& state) const;
  void redo(CommandManager& commands, GapBuffer& buffer, EditorState& state) const;
};

} // namespace mtx
