#include "mtx/menu_bar.hpp"
#include <algorithm>

namespace mtx {

MenuBar::MenuBar(int height) : height_(height) {}

void MenuBar::rebuild(const CommandRegistry& registry, int char_w) {
  cells_.clear();
  int x = 8;
  const int y = 0;
  const int h = height_;
  auto add_cell = [&](CommandID id, const std::string& group) {
    const auto* spec = registry.find(id);
    if (!spec) return;
    std::string label = group.empty() ? spec->name : group + ":" + spec->name;
    std::string shortcut = registry.shortcut_label(id);
    const int chars = static_cast<int>(label.size() + shortcut.size() + (shortcut.empty() ? 2 : 5));
    const int w = std::max(56, chars * std::max(1, char_w));
    cells_.push_back({x, y, w, h, id, label, shortcut});
    x += w + 4;
  };

  add_cell(CommandID::NewFile, "File");
  add_cell(CommandID::OpenFile, "File");
  add_cell(CommandID::Save, "File");
  add_cell(CommandID::SaveAs, "File");
  add_cell(CommandID::ExportHtml, "File");
  add_cell(CommandID::TogglePreview, "View");
  add_cell(CommandID::TogglePreviewLock, "View");
  add_cell(CommandID::OpenWorkspace, "Project");
  add_cell(CommandID::WorkspaceSearch, "Project");
  add_cell(CommandID::WorkspaceSymbols, "Project");
  add_cell(CommandID::ValidateLinks, "Project");
  add_cell(CommandID::ImagePathSuggestions, "Project");
  add_cell(CommandID::RunDiagnostics, "Project");
  add_cell(CommandID::GitStatus, "Project");
  add_cell(CommandID::Undo, "Edit");
  add_cell(CommandID::Redo, "Edit");
  add_cell(CommandID::Copy, "Edit");
  add_cell(CommandID::Cut, "Edit");
  add_cell(CommandID::Paste, "Edit");
  add_cell(CommandID::SelectAll, "Edit");
  add_cell(CommandID::Quit, "File");
}

bool MenuBar::contains(int, int y) const noexcept { return y >= 0 && y < height_; }

CommandID MenuBar::command_at(int x, int y) const noexcept {
  if (!contains(x, y)) return CommandID::NoCommand;
  for (const auto& c : cells_) {
    if (x >= c.x && x < c.x + c.w && y >= c.y && y < c.y + c.h) return c.command;
  }
  return CommandID::NoCommand;
}

} // namespace mtx
