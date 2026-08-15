#include "mtx/command_dispatcher.hpp"
#include "mtx/cursor_controller.hpp"
#include "mtx/glyph_metrics_table.hpp"
#include "mtx/highlight_processor.hpp"
#include "mtx/input_dispatcher.hpp"
#include "mtx/viewport_manager.hpp"
#include "mtx/command_registry.hpp"
#include "mtx/menu_bar.hpp"
#include "mtx/selection_engine.hpp"
#include "mtx/cursor_manager.hpp"
#include "mtx/context_projection.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <iostream>
#include <stdexcept>

static void test_cursor_view_highlight() {
  std::string t = "abc\ndef\n";
  auto idx = mtx::build_line_index(t);
  assert(mtx::byte_to_row_col(idx, 5).row == 1);
  assert(mtx::row_col_to_byte(idx, 1, 2) == 6);

  mtx::EditorState st;
  st.cursor_byte = 5;
  st.preferred_col = 1;
  mtx::CursorController cc;
  cc.move_up(st, t, false);
  assert(st.cursor_byte == 1);

  st.selection.active = true;
  st.selection.anchor = 1;
  st.selection.focus = 4;
  mtx::HighlightProcessor hp;
  auto attrs = hp.build("# h\n**b**\n", st.selection);
  assert(attrs.size() == 10);
}

static void test_dispatchers_and_metrics() {
  mtx::GapBuffer b("abc");
  mtx::EditorState st;
  st.cursor_byte = 1;
  mtx::CommandManager cm;
  mtx::CommandDispatcher d;
  d.insert_text(cm, b, st, "X");
  assert(b.str() == "aXbc");
  d.undo(cm, b, st);
  assert(b.str() == "abc");
  d.redo(cm, b, st);
  assert(b.str() == "aXbc");

  mtx::CommandRegistry reg;
  mtx::InputDispatcher input(&reg);
  auto new_file = input.translate({mtx::InputKey::Character, "n", true, false}, true);
  assert(new_file.kind == mtx::ActionKind::Command);
  assert(new_file.command == mtx::CommandID::NewFile);
  auto open_file = input.translate({mtx::InputKey::Character, "o", true, false}, true);
  assert(open_file.kind == mtx::ActionKind::Command);
  assert(open_file.command == mtx::CommandID::OpenFile);
  auto preview = input.translate({mtx::InputKey::Character, "p", true, false}, true);
  assert(preview.kind == mtx::ActionKind::Command);
  assert(preview.command == mtx::CommandID::TogglePreview);
  auto undo = input.translate({mtx::InputKey::Character, "z", true, false}, true);
  assert(undo.kind == mtx::ActionKind::Command);
  assert(undo.command == mtx::CommandID::Undo);
  auto export_printable = input.translate({mtx::InputKey::Character, "e", true, false}, true);
  assert(export_printable.kind == mtx::ActionKind::Command);
  assert(export_printable.command == mtx::CommandID::ExportHtml);
  auto export_control_byte = input.translate({mtx::InputKey::Character, std::string(1, char(0x05)), true, false}, true);
  assert(export_control_byte.kind == mtx::ActionKind::Command);
  assert(export_control_byte.command == mtx::CommandID::ExportHtml);
  auto save_control_byte = input.translate({mtx::InputKey::Character, std::string(1, char(0x13)), true, false}, true);
  assert(save_control_byte.kind == mtx::ActionKind::Command);
  assert(save_control_byte.command == mtx::CommandID::Save);
  auto preview_lock = input.translate({mtx::InputKey::Character, "p", true, true}, true);
  assert(preview_lock.kind == mtx::ActionKind::Command);
  assert(preview_lock.command == mtx::CommandID::TogglePreviewLock);
  auto nl = input.translate({mtx::InputKey::Enter, "", false, false}, true);
  assert(nl.kind == mtx::ActionKind::SmartNewline);

  mtx::GlyphMetricsTable gm;
  gm.set_fixed_width(8);
  assert(gm.measure_bytes("abcd", 1, 3) == 16);
  assert(gm.byte_at_x("abcd", 0, 4, 13) == 2);
  const std::string utf8 = u8"für";
  assert(gm.measure_bytes(utf8, 0, utf8.size()) == 24);
  assert(gm.byte_at_x(utf8, 0, utf8.size(), 6) == 1);
  assert(gm.byte_at_x(utf8, 0, utf8.size(), 14) == 3);
  assert(gm.byte_at_x(utf8, 0, utf8.size(), 22) == utf8.size());

  mtx::MenuBar mb;
  mb.rebuild(reg, 8);
  assert(reg.find(mtx::CommandID::NewFile) != nullptr);
  assert(reg.find(mtx::CommandID::OpenFile) != nullptr);
  assert(reg.find(mtx::CommandID::SaveAs) != nullptr);
  assert(reg.find(mtx::CommandID::TogglePreview) != nullptr);
  assert(reg.find(mtx::CommandID::TogglePreviewLock) != nullptr);
  assert(reg.find(mtx::CommandID::WorkspaceSearch) != nullptr);
  assert(reg.find(mtx::CommandID::WorkspaceSymbols) != nullptr);
  assert(reg.find(mtx::CommandID::ValidateLinks) != nullptr);
  assert(reg.find(mtx::CommandID::ImagePathSuggestions) != nullptr);
  assert(reg.find(mtx::CommandID::RunDiagnostics) != nullptr);
  assert(reg.find(mtx::CommandID::InsertSnippet) != nullptr);
  assert(reg.find(mtx::CommandID::InsertImage) != nullptr);
  assert(reg.find(mtx::CommandID::GitStatus) != nullptr);
  assert(reg.find(mtx::CommandID::OpenWorkspace) != nullptr);
  assert(reg.find(mtx::CommandID::ExportHtmlCyberpunk) != nullptr);
  assert(reg.find(mtx::CommandID::ExportHtmlDystopia) != nullptr);
  assert(reg.find(mtx::CommandID::ExportHtmlHorror) != nullptr);
  assert(reg.find(mtx::CommandID::ExportHtmlAdventure) != nullptr);
  assert(reg.find(mtx::CommandID::PreviewThemeCyberpunk) != nullptr);
  assert(reg.find(mtx::CommandID::PreviewThemeHorror) != nullptr);
  bool has_preview_cell = false;
  bool has_validation_cell = false;
  bool has_image_cell = false;
  bool has_diagnostics_cell = false;
  bool has_git_cell = false;
  for (const auto& cell : mb.cells()) {
    if (cell.command == mtx::CommandID::TogglePreview) has_preview_cell = true;
    if (cell.command == mtx::CommandID::ValidateLinks) has_validation_cell = true;
    if (cell.command == mtx::CommandID::ImagePathSuggestions) has_image_cell = true;
    if (cell.command == mtx::CommandID::RunDiagnostics) has_diagnostics_cell = true;
    if (cell.command == mtx::CommandID::GitStatus) has_git_cell = true;
  }
  assert(has_preview_cell);
  assert(has_validation_cell);
  assert(has_image_cell);
  assert(has_diagnostics_cell);
  assert(has_git_cell);
  assert(!mb.cells().empty());
  assert(mb.command_at(mb.cells()[0].x + 1, mb.cells()[0].y + 1) == mb.cells()[0].command);

  mtx::SelectionEngine se;
  se.begin_drag(st, 1);
  se.update_drag(st, 3);
  assert(st.selection.active && st.selection.begin() == 1 && st.selection.end() == 3);
  se.end_drag(st);

  mtx::CursorManager cv;
  mtx::Viewport vp;
  vp.rows = 10; vp.cols = 80;
  st.cursor_byte = 2;
  if (!cv.visual_for_byte("abcd", st, vp, gm, 10, 20).visible) {
    throw std::runtime_error("CursorVisual should be visible");
  }
  if (cv.visual_for_byte("abcd", st, vp, gm, 10, 20).x != 26) {
    throw std::runtime_error("CursorVisual x coordinate mismatch");
  }

  mtx::ContextProjection cp;
  mtx::Selection no_sel;
  auto ab = mtx::HighlightProcessor().build("# heading\n", no_sel);
  const std::string label = cp.token_label("# heading\n", 1, ab);
  if (label.find("heading") == std::string::npos) {
    throw std::runtime_error("ContextProjection failed to classify heading");
  }

}

int main() {
  test_cursor_view_highlight();
  test_dispatchers_and_metrics();
  std::cout << "ui_model ok\n";
}
