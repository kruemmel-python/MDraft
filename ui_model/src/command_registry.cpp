#include "mtx/command_registry.hpp"
#include <sstream>

namespace mtx {

static std::string label_for(KeyCombo k) {
  if (k.key == 0) return {};
  std::string s;
  if (k.ctrl) s += "Ctrl+";
  if (k.shift) s += "Shift+";
  if (k.alt) s += "Alt+";
  char c = static_cast<char>(k.key);
  if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
  s.push_back(c);
  return s;
}

void CommandRegistry::add(CommandID id, const std::string& name, const std::string& menu_path, KeyCombo shortcut) {
  specs_.push_back({id, name, menu_path, shortcut});
  if (shortcut.key != 0) shortcuts_[shortcut] = id;
}

CommandRegistry::CommandRegistry() {
  add(CommandID::NewFile,    "New",         "File/New",         {'n', true, false, false});
  add(CommandID::OpenFile,   "Open",        "File/Open",        {'o', true, false, false});
  add(CommandID::Save,       "Save",        "File/Save",        {'s', true, false, false});
  add(CommandID::SaveAs,     "Save As",     "File/Save As",     {0, false, false, false});
  add(CommandID::ExportHtml,          "Export HTML Standard", "File/Export HTML/Standard", {'e', true, false, false});
  add(CommandID::ExportHtmlCyberpunk, "Cyberpunk",            "File/Export HTML/Cyberpunk", {0, false, false, false});
  add(CommandID::ExportHtmlDystopia,  "Dystopie",             "File/Export HTML/Dystopie", {0, false, false, false});
  add(CommandID::ExportHtmlHorror,    "Horror",               "File/Export HTML/Horror", {0, false, false, false});
  add(CommandID::ExportHtmlAdventure, "Abenteuer",            "File/Export HTML/Abenteuer", {0, false, false, false});
  add(CommandID::PreviewThemeStandard,  "Preview Standard",  "View/Preview Theme/Standard", {0, false, false, false});
  add(CommandID::PreviewThemeCyberpunk, "Preview Cyberpunk", "View/Preview Theme/Cyberpunk", {0, false, false, false});
  add(CommandID::PreviewThemeDystopia,  "Preview Dystopie",  "View/Preview Theme/Dystopie", {0, false, false, false});
  add(CommandID::PreviewThemeHorror,    "Preview Horror",    "View/Preview Theme/Horror", {0, false, false, false});
  add(CommandID::PreviewThemeAdventure, "Preview Abenteuer", "View/Preview Theme/Abenteuer", {0, false, false, false});
  add(CommandID::OpenWorkspace,    "Open Workspace",    "Project/Open Workspace", {0, false, false, false});
  add(CommandID::ReindexWorkspace, "Reindex Workspace", "Project/Reindex Workspace", {'r', true, true, false});
  add(CommandID::WorkspaceStatus,  "Workspace Status",  "Project/Workspace Status", {0, false, false, false});
  add(CommandID::WorkspaceSearch,  "Workspace Search",  "Project/Search", {'f', true, true, false});
  add(CommandID::WorkspaceSymbols, "Workspace Symbols", "Project/Symbol Navigation", {'o', true, true, false});
  add(CommandID::ValidateLinks, "Validate Links", "Project/Validate Links", {'l', true, true, false});
  add(CommandID::ImagePathSuggestions, "Image Path Suggestions", "Project/Image Path Suggestions", {'i', true, true, false});
  add(CommandID::RunDiagnostics, "Diagnostics/Linting", "Project/Diagnostics Linting", {'d', true, true, false});
  add(CommandID::GitStatus, "Git Status Light", "Project/Git Status", {'g', true, true, false});
  add(CommandID::InsertSnippet, "Insert Snippet", "Edit/Insert Snippet", {'j', true, false, false});
  add(CommandID::InsertImage, "Insert Image", "Edit/Insert Image", {0, false, false, false});
  add(CommandID::Quit,       "Quit",        "File/Quit",        {'q', true, false, false});
  add(CommandID::Undo,       "Undo",        "Edit/Undo",        {'z', true, false, false});
  add(CommandID::Redo,       "Redo",        "Edit/Redo",        {'y', true, false, false});
  add(CommandID::ToggleMode, "Toggle Mode", "Edit/Toggle Mode", {0, false, false, false});
  add(CommandID::TogglePreview, "RenderIR/HTML Live Preview", "View/RenderIR HTML Live Preview", {'p', true, false, false});
  add(CommandID::TogglePreviewLock, "Lock Preview", "View/Lock Preview", {'p', true, true, false});
  add(CommandID::Copy,       "Copy",        "Edit/Copy",        {'c', true, false, false});
  add(CommandID::Cut,        "Cut",         "Edit/Cut",         {'x', true, false, false});
  add(CommandID::Paste,      "Paste",       "Edit/Paste",       {'v', true, false, false});
  add(CommandID::SelectAll,  "Select All",  "Edit/Select All",  {'a', true, false, false});
  add(CommandID::About,      "About",       "Help/About",       {0, false, false, false});
}

const CommandSpec* CommandRegistry::find(CommandID id) const {
  for (const auto& s : specs_) if (s.id == id) return &s;
  return nullptr;
}

CommandID CommandRegistry::lookup(const KeyCombo& combo) const {
  auto it = shortcuts_.find(combo);
  return it == shortcuts_.end() ? CommandID::NoCommand : it->second;
}

std::string CommandRegistry::shortcut_label(CommandID id) const {
  const CommandSpec* s = find(id);
  return s ? label_for(s->shortcut) : std::string{};
}

std::string command_name(CommandID id) {
  switch (id) {
    case CommandID::NewFile: return "New";
    case CommandID::OpenFile: return "Open";
    case CommandID::Save: return "Save";
    case CommandID::SaveAs: return "Save As";
    case CommandID::ExportHtml: return "Export HTML Standard";
    case CommandID::ExportHtmlCyberpunk: return "Export HTML Cyberpunk";
    case CommandID::ExportHtmlDystopia: return "Export HTML Dystopie";
    case CommandID::ExportHtmlHorror: return "Export HTML Horror";
    case CommandID::ExportHtmlAdventure: return "Export HTML Abenteuer";
    case CommandID::PreviewThemeStandard: return "Preview Theme Standard";
    case CommandID::PreviewThemeCyberpunk: return "Preview Theme Cyberpunk";
    case CommandID::PreviewThemeDystopia: return "Preview Theme Dystopie";
    case CommandID::PreviewThemeHorror: return "Preview Theme Horror";
    case CommandID::PreviewThemeAdventure: return "Preview Theme Abenteuer";
    case CommandID::OpenWorkspace: return "Open Workspace";
    case CommandID::ReindexWorkspace: return "Reindex Workspace";
    case CommandID::WorkspaceStatus: return "Workspace Status";
    case CommandID::WorkspaceSearch: return "Workspace Search";
    case CommandID::WorkspaceSymbols: return "Workspace Symbols";
    case CommandID::ValidateLinks: return "Validate Links";
    case CommandID::ImagePathSuggestions: return "Image Path Suggestions";
    case CommandID::RunDiagnostics: return "Diagnostics/Linting";
    case CommandID::GitStatus: return "Git Status Light";
    case CommandID::InsertSnippet: return "Insert Snippet";
    case CommandID::InsertImage: return "Insert Image";
    case CommandID::Quit: return "Quit";
    case CommandID::Undo: return "Undo";
    case CommandID::Redo: return "Redo";
    case CommandID::ToggleMode: return "Toggle Mode";
    case CommandID::TogglePreview: return "RenderIR/HTML Live Preview";
    case CommandID::TogglePreviewLock: return "Lock Preview";
    case CommandID::Copy: return "Copy";
    case CommandID::Cut: return "Cut";
    case CommandID::Paste: return "Paste";
    case CommandID::SelectAll: return "Select All";
    case CommandID::About: return "About";
    default: return "None";
  }
}

} // namespace mtx
