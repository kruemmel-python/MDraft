#pragma once
#include "mtx/command_manager.hpp"
#include "mtx/command_registry.hpp"
#include "mtx/command_dispatcher.hpp"
#include "mtx/editor_state.hpp"
#include "mtx/clipboard_bridge.hpp"
#include "mtx/gap_buffer.hpp"
#include "mtx/selection_engine.hpp"
#include <string>

namespace mtx {

// Runtime-owned mutable editor kernel. GUI layers may request commands, but they do
// not perform IO, export, clipboard mutation, or core-buffer edits directly.
class EditorRuntime {
public:
  EditorRuntime(GapBuffer& buffer,
                EditorState& state,
                CommandManager& commands,
                std::string path);

  GapBuffer& buffer() noexcept { return buffer_; }
  const GapBuffer& buffer() const noexcept { return buffer_; }

  EditorState& state() noexcept { return state_; }
  const EditorState& state() const noexcept { return state_; }

  const std::string& path() const noexcept { return path_; }
  void new_document(const std::string& path = std::string{});
  void open_document(const std::string& path, const std::string& text);
  void save_as(const std::string& path);
  const std::string& clipboard() const noexcept { return clipboard_; }
  void set_clipboard_bridge(ClipboardBridge* bridge) noexcept { clipboard_bridge_ = bridge; }
  bool can_undo() const noexcept;
  bool can_redo() const noexcept;

  bool execute(CommandID id, bool& running);

  void insert_text(const std::string& text);
  void smart_newline();
  void backspace();
  void delete_at();
  void undo();
  void redo();
  void toggle_mode();
  void select_all();

private:
  GapBuffer& buffer_;
  EditorState& state_;
  CommandManager& commands_;
  std::string path_;
  std::string clipboard_;
  ClipboardBridge* clipboard_bridge_{nullptr};

  CommandDispatcher text_commands_;
  SelectionEngine selection_;

  void mark_status(const std::string& s);
};

} // namespace mtx
