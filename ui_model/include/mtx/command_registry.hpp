#pragma once
#include <string>
#include <unordered_map>
#include <vector>

namespace mtx {

enum class CommandID {
  NoCommand,
  NewFile,
  OpenFile,
  Save,
  SaveAs,
  ExportHtml,
  ExportHtmlGitHub,
  ExportHtmlCyberpunk,
  ExportHtmlDystopia,
  ExportHtmlHorror,
  ExportHtmlAdventure,
  PreviewThemeStandard,
  PreviewThemeGitHub,
  PreviewThemeCyberpunk,
  PreviewThemeDystopia,
  PreviewThemeHorror,
  PreviewThemeAdventure,
  OpenWorkspace,
  ReindexWorkspace,
  WorkspaceStatus,
  WorkspaceSearch,
  WorkspaceSymbols,
  ValidateLinks,
  ImagePathSuggestions,
  RunDiagnostics,
  GitStatus,
  InsertSnippet,
  InsertImage,
  Quit,
  Undo,
  Redo,
  ToggleMode,
  TogglePreview,
  TogglePreviewLock,
  Copy,
  Cut,
  Paste,
  SelectAll,
  About
};

struct KeyCombo {
  unsigned int key{0};      // lower-case ASCII for character shortcuts
  bool ctrl{false};
  bool shift{false};
  bool alt{false};

  bool operator==(const KeyCombo& o) const noexcept {
    return key == o.key && ctrl == o.ctrl && shift == o.shift && alt == o.alt;
  }
};

struct KeyComboHash {
  std::size_t operator()(const KeyCombo& k) const noexcept {
    return (static_cast<std::size_t>(k.key) << 3) ^
           (k.ctrl ? 1u : 0u) ^ (k.shift ? 2u : 0u) ^ (k.alt ? 4u : 0u);
  }
};

struct CommandSpec {
  CommandID id{CommandID::NoCommand};
  std::string name;
  std::string menu_path;
  KeyCombo shortcut{};
};

class CommandRegistry {
public:
  CommandRegistry();

  const CommandSpec* find(CommandID id) const;
  CommandID lookup(const KeyCombo& combo) const;
  std::string shortcut_label(CommandID id) const;
  const std::vector<CommandSpec>& commands() const noexcept { return specs_; }

private:
  std::vector<CommandSpec> specs_;
  std::unordered_map<KeyCombo, CommandID, KeyComboHash> shortcuts_;

  void add(CommandID id, const std::string& name, const std::string& menu_path, KeyCombo shortcut);
};

std::string command_name(CommandID id);

} // namespace mtx
