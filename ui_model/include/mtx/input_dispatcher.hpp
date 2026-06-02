#pragma once
#include "mtx/command_registry.hpp"
#include <string>

namespace mtx {

enum class InputKey {
  NoInput, Character, Enter, Backspace, DeleteKey,
  Left, Right, Up, Down, PageUp, PageDown, Escape
};

struct InputEvent {
  InputKey key{InputKey::NoInput};
  std::string text;
  bool ctrl{false};
  bool shift{false};
};

enum class ActionKind {
  NoAction, InsertText, SmartNewline, Backspace, DeleteAt,
  MoveLeft, MoveRight, MoveUp, MoveDown, PageUp, PageDown,
  ToggleMode, Save, ExportHtml, Quit, Undo, Redo, Command
};

struct EditorAction {
  ActionKind kind{ActionKind::NoAction};
  std::string text;
  bool select{false};
  CommandID command{CommandID::NoCommand};
};

class InputDispatcher {
public:
  explicit InputDispatcher(const CommandRegistry* registry = nullptr) : registry_(registry) {}
  void bind_registry(const CommandRegistry* registry) { registry_ = registry; }
  EditorAction translate(const InputEvent& ev, bool insert_mode) const;
private:
  const CommandRegistry* registry_{nullptr};
};

} // namespace mtx
