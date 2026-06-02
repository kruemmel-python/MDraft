#include "mtx/input_dispatcher.hpp"

namespace mtx {

EditorAction InputDispatcher::translate(const InputEvent& ev, bool insert_mode) const {
  if (ev.ctrl && ev.key == InputKey::Character && ev.text.size() == 1) {
    unsigned char raw = static_cast<unsigned char>(ev.text[0]);
    char c = 0;
    // X11 often reports Ctrl+A..Ctrl+Z as control bytes 0x01..0x1a instead of printable ASCII.
    // The editor ABI must normalize this before querying the shortcut registry.
    if (raw >= 1 && raw <= 26) c = static_cast<char>('a' + raw - 1);
    else c = static_cast<char>(raw | 0x20);
    const CommandID cid = registry_ ? registry_->lookup({static_cast<unsigned int>(c), true, ev.shift, false}) : CommandID::NoCommand;
    if (cid != CommandID::NoCommand) return {ActionKind::Command, {}, false, cid};

    // Compatibility fallback if a registry was not bound.
    if (c == 's') return {ActionKind::Command, {}, false, CommandID::Save};
    if (c == 'e') return {ActionKind::Command, {}, false, CommandID::ExportHtml};
    if (c == 'q') return {ActionKind::Command, {}, false, CommandID::Quit};
    if (c == 'z') return {ActionKind::Command, {}, false, CommandID::Undo};
    if (c == 'y') return {ActionKind::Command, {}, false, CommandID::Redo};
  }

  switch (ev.key) {
    case InputKey::Escape: return {ActionKind::Command, {}, false, CommandID::ToggleMode};
    case InputKey::Left: return {ActionKind::MoveLeft, {}, ev.shift, CommandID::NoCommand};
    case InputKey::Right: return {ActionKind::MoveRight, {}, ev.shift, CommandID::NoCommand};
    case InputKey::Up: return {ActionKind::MoveUp, {}, ev.shift, CommandID::NoCommand};
    case InputKey::Down: return {ActionKind::MoveDown, {}, ev.shift, CommandID::NoCommand};
    case InputKey::PageUp: return {ActionKind::PageUp, {}, false, CommandID::NoCommand};
    case InputKey::PageDown: return {ActionKind::PageDown, {}, false, CommandID::NoCommand};
    default: break;
  }

  if (!insert_mode) return {ActionKind::NoAction, {}, false, CommandID::NoCommand};
  if (ev.key == InputKey::Enter) return {ActionKind::SmartNewline, {}, false, CommandID::NoCommand};
  if (ev.key == InputKey::Backspace) return {ActionKind::Backspace, {}, false, CommandID::NoCommand};
  if (ev.key == InputKey::DeleteKey) return {ActionKind::DeleteAt, {}, false, CommandID::NoCommand};
  if (ev.key == InputKey::Character && !ev.text.empty()) return {ActionKind::InsertText, ev.text, false, CommandID::NoCommand};
  return {ActionKind::NoAction, {}, false, CommandID::NoCommand};
}

} // namespace mtx
