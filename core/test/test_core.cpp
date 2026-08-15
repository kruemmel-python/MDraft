#include "mtx/command_manager.hpp"
#include "mtx/gap_buffer.hpp"
#include "mtx/markdown.hpp"
#include "mtx/html.hpp"
#include "mtx/layout_engine.hpp"
#include "mtx/render_svg.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <iostream>
#include <stdexcept>

static void test_gap_and_markdown() {
  mtx::GapBuffer b("abc");
  b.move_to(1);
  b.insert('X');
  assert(b.str() == "aXbc");
  b.erase_range(1,3);
  assert(b.str() == "ac");

  auto blocks = mtx::parse_blocks("# H\n- [x] t\n$$\nx\n$$\n");
  assert(blocks.size() >= 5);
  auto spans = mtx::parse_inline_styles("# H\n**b** `c` $m$\n");
  assert(!spans.empty());
}


static void test_render_ir_svg_html() {
  const std::string md =
    "# Title\n\n"
    "Text with **bold**, *italic*, `code` and $a^2$.\n\n"
    "$$\n"
    "E = mc^2\n"
    "$$\n\n"
    "| A | B |\n"
    "|---|---|\n"
    "| 1 | 2 |\n\n"
    "```mermaid\n"
    "flowchart LR\n"
    "A[Input] -->|parse| B(AST)\n"
    "B --> C{HTML}\n"
    "```\n\n"
    "![Logo](images/logo.png)\n";

  const mtx::DisplayList a = mtx::markdown_to_display_list(md, mtx::HtmlTheme::Horror, 760);
  const mtx::DisplayList b = mtx::markdown_to_display_list(md, mtx::HtmlTheme::Horror, 760);
  assert(a.width == 760);
  assert(a.height > 100);
  // Table state-machine must not collapse pipe rows into a paragraph.
  bool table_grid_seen = false;
  for (const auto& cmd : a.commands) {
    if (cmd.kind == mtx::DrawKind::Text && cmd.text == "A") table_grid_seen = true;
  }
  assert(table_grid_seen);
  assert(a.commands.size() == b.commands.size());
  assert(mtx::display_list_hash(a) == mtx::display_list_hash(b));
  assert(a.math_box_count == 1);
  assert(a.mermaid_node_count == 3);
  assert(a.mermaid_edge_count == 2);
  assert(a.image_count == 1);

  const std::string svg_html = mtx::display_list_to_svg_html(a);
  assert(svg_html.find("<svg") != std::string::npos);
  assert(svg_html.find("Title") != std::string::npos);
  assert(svg_html.find("Input") != std::string::npos);
  assert(svg_html.find("theme-horror") != std::string::npos);
  assert(svg_html.find("<image") != std::string::npos);
  assert(svg_html.find("images/logo.png") != std::string::npos);
  assert(svg_html.find("mc²") != std::string::npos || svg_html.find("mc") != std::string::npos);

  const std::string html = mtx::render_html(md, mtx::HtmlTheme::Horror);
  assert(html.find("<svg") != std::string::npos);
  assert(html.find("MDraft RenderIR Export") != std::string::npos);
}

static void test_preview_layout_wrap_and_contrast() {
  const std::string long_title =
    "PlayerLog NG 2.0.0 - bebilderte Ingame-Anleitung fuer Operatoren mit sehr langer Ueberschrift";
  const std::string md =
    "# " + long_title + "\n\n"
    "1. Im Spiel den Chat oeffnen.\n"
    "2. Den Befehl einschliesslich des fuehrenden `/` eingeben.\n"
    "3. Mit Enter absenden.\n\n"
    "## Pruefen, ob PlayerLog NG aktiv ist\n\n"
    "```text\n"
    "/plugins\n"
    "```\n";

  const mtx::DisplayList list = mtx::markdown_to_display_list(md, mtx::HtmlTheme::Standard, 420);
  int h1_lines = 0;
  bool heading_contrast_ok = true;
  bool first_item_seen = false;
  bool second_item_seen = false;

  for (const auto& cmd : list.commands) {
    if (cmd.kind != mtx::DrawKind::Text) continue;
    if (cmd.weight == mtx::TextWeight::Bold && cmd.font_size >= 20) {
      if (long_title.find(cmd.text) != std::string::npos) ++h1_lines;
      if (cmd.fill.r > 240 && cmd.fill.g > 240 && cmd.fill.b > 240) heading_contrast_ok = false;
      const int estimated_right = cmd.x + static_cast<int>(cmd.text.size()) * std::max(8, (cmd.font_size * 6) / 10);
      assert(estimated_right <= list.width + 8);
    }
    if (cmd.text.find("Im Spiel den Chat") != std::string::npos) first_item_seen = true;
    if (cmd.text.find("Den Befehl") != std::string::npos) second_item_seen = true;
  }

  assert(h1_lines >= 2);
  assert(heading_contrast_ok);
  assert(first_item_seen);
  assert(second_item_seen);
}

static void test_command_manager_undo_redo() {
  mtx::GapBuffer b("ab");
  mtx::CommandManager cm;
  auto r = cm.insert(b, 1, "X");
  assert(r.changed && r.cursor == 2);
  assert(b.str() == "aXb");
  r = cm.undo(b, r.cursor);
  assert(r.changed && r.cursor == 1);
  assert(b.str() == "ab");
  r = cm.redo(b, r.cursor);
  assert(r.changed && r.cursor == 2);
  assert(b.str() == "aXb");
}

static void test_command_manager_smart_newline() {
  mtx::GapBuffer b("- item");
  mtx::CommandManager cm;
  if (!cm.smart_newline(b, b.size()).changed) {
    throw std::runtime_error("smart_newline did not change unordered list");
  }
  assert(b.str() == "- item\n- ");
  mtx::GapBuffer t("- [x] done");
  if (!cm.smart_newline(t, t.size()).changed) {
    throw std::runtime_error("smart_newline did not change task list");
  }
  assert(t.str() == "- [x] done\n- [ ] ");
  mtx::GapBuffer ordered("9. nine");
  mtx::CommandManager cm2;
  if (!cm2.smart_newline(ordered, ordered.size()).changed) {
    throw std::runtime_error("smart_newline did not change ordered list");
  }
  assert(ordered.str() == "9. nine\n10. ");
}

int main() {
  test_gap_and_markdown();
  test_render_ir_svg_html();
  test_preview_layout_wrap_and_contrast();
  test_command_manager_undo_redo();
  test_command_manager_smart_newline();
  std::cout << "core ok\n";
}
