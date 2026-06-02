#include "mtx/snippets.hpp"
#include <algorithm>
#include <sstream>

namespace mtx {
namespace {

std::size_t remove_cursor_marker(std::string& s) {
  const std::string marker = "${cursor}";
  const std::size_t p = s.find(marker);
  if (p == std::string::npos) return s.size();
  s.erase(p, marker.size());
  return p;
}

} // namespace

const std::vector<Snippet>& default_snippets() {
  static const std::vector<Snippet> snippets = {
    {"h1", "Heading 1", "# ${cursor}\n", "Überschrift Ebene 1"},
    {"h2", "Heading 2", "## ${cursor}\n", "Überschrift Ebene 2"},
    {"todo", "Task List", "- [ ] ${cursor}\n", "GitHub-kompatible Task"},
    {"table", "Markdown Table", "| A | B |\n|---|---|\n| ${cursor} |  |\n", "Pipe-Tabelle"},
    {"math", "Math Block", "$$\n${cursor}\n$$\n", "Mathe-Block"},
    {"mermaid", "Mermaid Flowchart", "```mermaid\nflowchart LR\nA[Input] -->|parse| B[RenderIR]\nB --> C[HTML/SVG]\n${cursor}\n```\n", "Mermaid-Diagramm"},
    {"code", "Code Fence", "```cpp\n${cursor}\n```\n", "Codeblock"},
    {"img", "Markdown Image", "![${cursor}](assets/image.png)\n", "Bildreferenz"},
    {"note", "Note Block", "> **Notiz:** ${cursor}\n", "Zitat/Notiz"}
  };
  return snippets;
}

SnippetExpansion expand_snippet_trigger(const std::string& trigger) {
  for (const auto& snippet : default_snippets()) {
    if (snippet.trigger == trigger) {
      SnippetExpansion out;
      out.matched = true;
      out.trigger = trigger;
      out.body = snippet.body;
      out.cursor_offset = remove_cursor_marker(out.body);
      return out;
    }
  }
  return {};
}

std::string format_snippet_catalog() {
  std::ostringstream o;
  o << "MDraft Snippets\n\n";
  for (const auto& snippet : default_snippets()) {
    o << snippet.trigger << "  ->  " << snippet.label << "\n";
    o << "    " << snippet.description << "\n";
  }
  return o.str();
}

std::string make_markdown_image(const std::string& alt, const std::string& href) {
  return "![" + alt + "](" + href + ")";
}

} // namespace mtx
