#include "mtx/editor_runtime.hpp"
#include "mtx/file_io.hpp"
#include "mtx/html.hpp"
#include <stdexcept>
#include <utility>

namespace mtx {

EditorRuntime::EditorRuntime(GapBuffer& buffer,
                             EditorState& state,
                             CommandManager& commands,
                             std::string path)
  : buffer_(buffer), state_(state), commands_(commands), path_(std::move(path)) {}

void EditorRuntime::mark_status(const std::string& s) {
  state_.status = s;
}

void EditorRuntime::new_document(const std::string& path) {
  path_ = path;
  buffer_.set_text("");
  commands_.clear();
  state_ = EditorState{};
  state_.mode = EditorMode::Insert;
  state_.dirty = false;
  mark_status(path_.empty() ? "Neue Markdown-Datei" : "Neue Markdown-Datei: " + path_);
}

void EditorRuntime::open_document(const std::string& path, const std::string& text) {
  path_ = path;
  buffer_.set_text(text);
  commands_.clear();
  state_ = EditorState{};
  state_.mode = EditorMode::Insert;
  state_.dirty = false;
  mark_status("Geöffnet: " + path_);
}

void EditorRuntime::save_as(const std::string& path) {
  path_ = path;
  write_file_atomic(path_, buffer_.str());
  commands_.clear();
  state_.dirty = false;
  mark_status("Gespeichert unter: " + path_);
}

bool EditorRuntime::can_undo() const noexcept {
  return commands_.can_undo();
}

bool EditorRuntime::can_redo() const noexcept {
  return commands_.can_redo();
}

void EditorRuntime::insert_text(const std::string& text) {
  text_commands_.insert_text(commands_, buffer_, state_, text);
}

void EditorRuntime::smart_newline() {
  text_commands_.smart_newline(commands_, buffer_, state_);
}

void EditorRuntime::backspace() {
  text_commands_.backspace(commands_, buffer_, state_);
}

void EditorRuntime::delete_at() {
  text_commands_.delete_at(commands_, buffer_, state_);
}

void EditorRuntime::undo() {
  text_commands_.undo(commands_, buffer_, state_);
}

void EditorRuntime::redo() {
  text_commands_.redo(commands_, buffer_, state_);
}

void EditorRuntime::toggle_mode() {
  state_.mode = (state_.mode == EditorMode::Insert) ? EditorMode::Command : EditorMode::Insert;
  mark_status(state_.mode == EditorMode::Insert ? "Modus: INSERT" : "Modus: COMMAND");
}

void EditorRuntime::select_all() {
  selection_.select_all(state_, buffer_.size());
  mark_status("Alles markiert");
}

static std::string themed_export_path(const std::string& path, HtmlTheme theme) {
  if (path.empty()) {
    throw std::runtime_error("kein Speicherpfad gesetzt");
  }
  if (theme == HtmlTheme::Standard) {
    return path + ".html";
  }
  return path + "." + std::string(html_theme_id(theme)) + ".html";
}

static void export_html_theme(const std::string& path,
                              const GapBuffer& buffer,
                              HtmlTheme theme,
                              EditorRuntime& runtime) {
  const std::string out = themed_export_path(path, theme);
  write_file_atomic(out, render_html(buffer.str(), theme));
  runtime.state().status = std::string("HTML exportiert [") + html_theme_label(theme) + "]: " + out;
}

bool EditorRuntime::execute(CommandID id, bool& running) {
  switch (id) {
    case CommandID::Quit:
      running = false;
      return true;

    case CommandID::NewFile:
      new_document();
      return true;

    case CommandID::OpenFile:
      mark_status("Öffnen wird von der UI-Projektion ausgeführt");
      return true;

    case CommandID::Save:
      if (path_.empty()) {
        throw std::runtime_error("kein Speicherpfad gesetzt");
      }
      write_file_atomic(path_, buffer_.str());
      commands_.clear();
      state_.dirty = false;
      mark_status("Gespeichert: " + path_);
      return true;

    case CommandID::SaveAs:
      if (path_.empty()) {
        throw std::runtime_error("kein Speicherpfad gesetzt");
      }
      save_as(path_);
      return true;

    case CommandID::ExportHtml:
      export_html_theme(path_, buffer_, HtmlTheme::Standard, *this);
      return true;

    case CommandID::ExportHtmlCyberpunk:
      export_html_theme(path_, buffer_, HtmlTheme::Cyberpunk, *this);
      return true;

    case CommandID::ExportHtmlDystopia:
      export_html_theme(path_, buffer_, HtmlTheme::Dystopia, *this);
      return true;

    case CommandID::ExportHtmlHorror:
      export_html_theme(path_, buffer_, HtmlTheme::Horror, *this);
      return true;

    case CommandID::ExportHtmlAdventure:
      export_html_theme(path_, buffer_, HtmlTheme::Adventure, *this);
      return true;

    case CommandID::Undo:
      undo();
      return true;

    case CommandID::Redo:
      redo();
      return true;

    case CommandID::ToggleMode:
      toggle_mode();
      return true;

    case CommandID::SelectAll:
      select_all();
      return true;

    case CommandID::About:
      mark_status("MDraft v0.10.3 - link validation and image suggestions substrate");
      return true;

    case CommandID::Copy:
      if (state_.selection.active && state_.selection.begin() != state_.selection.end()) {
        const std::string t = buffer_.str();
        clipboard_ = t.substr(state_.selection.begin(), state_.selection.end() - state_.selection.begin());
        bool external_ok = true;
        if (clipboard_bridge_ != nullptr) {
          external_ok = clipboard_bridge_->write_text(clipboard_);
        }
        mark_status(std::string("Kopiert: ") + std::to_string(clipboard_.size()) +
                    " Bytes" + (external_ok ? "" : " (intern)"));
      }
      return true;

    case CommandID::Cut:
      if (state_.selection.active && state_.selection.begin() != state_.selection.end()) {
        const std::string t = buffer_.str();
        clipboard_ = t.substr(state_.selection.begin(), state_.selection.end() - state_.selection.begin());
        bool external_ok = true;
        if (clipboard_bridge_ != nullptr) {
          external_ok = clipboard_bridge_->write_text(clipboard_);
        }
        delete_at(); // deletes active selection through CommandDispatcher.
        mark_status(std::string("Ausgeschnitten: ") + std::to_string(clipboard_.size()) +
                    " Bytes" + (external_ok ? "" : " (intern)"));
      }
      return true;

    case CommandID::Paste: {
      std::string incoming;
      if (clipboard_bridge_ != nullptr) {
        const bool external_ok = clipboard_bridge_->read_text(incoming);
        if (!external_ok) {
          incoming.clear();
        }
      }
      if (incoming.empty()) {
        incoming = clipboard_;
      }
      if (!incoming.empty()) {
        insert_text(incoming);
        clipboard_ = incoming;
        mark_status("Eingefuegt: " + std::to_string(incoming.size()) + " Bytes");
      } else {
        mark_status("Zwischenablage leer");
      }
      return true;
    }

    default:
      return false;
  }
}

} // namespace mtx
