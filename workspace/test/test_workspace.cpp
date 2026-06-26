#include "mtx/file_index.hpp"
#include "mtx/workspace_search.hpp"
#include "mtx/workspace_symbol_nav.hpp"
#include "mtx/link_validation.hpp"
#include "mtx/image_suggestions.hpp"
#include "mtx/diagnostics.hpp"
#include "mtx/workspace_lint.hpp"
#include "mtx/git_adapter.hpp"
#include "mtx/snippets.hpp"
#include "mtx/file_io.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

int main() {
  const fs::path root = fs::current_path() / "mdraft_workspace_test";
  fs::remove_all(root);
  fs::create_directories(root / "docs");
  fs::create_directories(root / "images");
  mtx::write_file_atomic((root / "index.md").generic_string(),
                         "# Root\n\nThis document contains alpha needle.\n\n[Doc](docs/doc.md)\n\n![Logo](images/logo.png)\n");
  mtx::write_file_atomic((root / "docs" / "doc.md").generic_string(),
                         "# Doc\n\n### Jumped heading\n\nneedle occurs here.  \n\n[Missing](missing.md)\n\n![Missing Image](missing.png)\n\n![](images/logo.png)\n");
  mtx::write_file_atomic((root / "images" / "logo.png").generic_string(), "png");
  mtx::write_file_atomic((root / "images" / "diagram.svg").generic_string(), "<svg/>");
  mtx::write_file_atomic((root / "ignore.bin").generic_string(), "needle ignored");

  const mtx::WorkspaceIndex idx = mtx::build_workspace_index(root.generic_string());
  assert(idx.files.size() == 2);
  assert(idx.symbols.size() == 3);
  assert(idx.links.size() == 5);
  assert(idx.assets.size() >= 3);
  assert(!idx.diagnostics.empty());
  assert(mtx::workspace_summary(idx).find("files=2") != std::string::npos);
  assert(mtx::workspace_summary(idx).find("assets=") != std::string::npos);

  const auto report = mtx::validate_workspace_links(idx);
  assert(report.links_total == 5);
  assert(report.images_total == 3);
  assert(report.links_unresolved >= 2);
  assert(report.images_unresolved >= 1);
  assert(mtx::format_link_validation_report(report).find("Link-Validierung") != std::string::npos);

  const auto suggestions = mtx::suggest_workspace_images(idx, "index.md", "logo");
  assert(!suggestions.empty());
  assert(suggestions[0].path.find("logo.png") != std::string::npos);
  assert(mtx::format_image_suggestions(suggestions, "logo").find("Bildpfad-Vorschläge") != std::string::npos);

  const auto hits = mtx::search_workspace(idx, "needle");
  assert(hits.size() == 2);
  assert(hits[0].file == "docs/doc.md" || hits[0].file == "index.md");
  assert(mtx::format_search_results(hits, "needle").find("Treffer=2") != std::string::npos);

  const auto symbols_doc = mtx::find_workspace_symbols(idx, "doc");
  assert(!symbols_doc.empty());
  assert(symbols_doc[0].title == "Doc" || symbols_doc[0].file == "docs/doc.md");
  const std::string symbol_list = mtx::format_symbol_results(symbols_doc, "doc");
  assert(symbol_list.find("Symbol-Navigation") != std::string::npos);
  assert(symbol_list.find("Treffer=") != std::string::npos);

  const auto all_symbols = mtx::find_workspace_symbols(idx, "");
  assert(all_symbols.size() == idx.symbols.size());

  const std::vector<mtx::Diagnostic> doc_lint =
    mtx::lint_markdown_document("doc.md", "# A\n\n### Jump\ntext  \n![](x.png)\n");
  assert(doc_lint.size() >= 3);
  bool saw_jump = false;
  bool saw_trailing = false;
  bool saw_alt = false;
  for (const auto& d : doc_lint) {
    if (d.code == "MD004") saw_jump = true;
    if (d.code == "MD001") saw_trailing = true;
    if (d.code == "MD006") saw_alt = true;
  }
  assert(saw_jump && saw_trailing && saw_alt);

  const auto lint = mtx::lint_workspace(idx);
  assert(!lint.diagnostics.empty());
  assert(lint.warning_count > 0);
  const std::string lint_text = mtx::format_lint_report(lint);
  assert(lint_text.find("Diagnostics/Linting") != std::string::npos);
  assert(lint_text.find("MD001") != std::string::npos || lint_text.find("MD004") != std::string::npos);



  const mtx::SnippetExpansion table_snippet = mtx::expand_snippet_trigger("table");
  assert(table_snippet.matched);
  assert(table_snippet.body.find("|---|") != std::string::npos);
  assert(table_snippet.cursor_offset < table_snippet.body.size());
  assert(mtx::expand_snippet_trigger("does-not-exist").matched == false);
  assert(mtx::format_snippet_catalog().find("mermaid") != std::string::npos);
  assert(mtx::make_markdown_image("Logo", "images/logo.png") == "![Logo](images/logo.png)");

  const mtx::GitFileStatus modified = mtx::parse_git_status_porcelain_line(" M docs/doc.md");
  assert(modified.kind == mtx::GitChangeKind::Modified);
  assert(modified.path == "docs/doc.md");
  const mtx::GitFileStatus added = mtx::parse_git_status_porcelain_line("A  new.md");
  assert(added.kind == mtx::GitChangeKind::Added);
  const mtx::GitFileStatus renamed = mtx::parse_git_status_porcelain_line("R  old.md -> new.md");
  assert(renamed.kind == mtx::GitChangeKind::Renamed);
  assert(renamed.old_path == "old.md");
  assert(renamed.path == "new.md");
  mtx::GitStatusReport git_report;
  git_report.repository = true;
  git_report.git_available = true;
  git_report.root = root.generic_string();
  git_report.files.push_back(modified);
  git_report.modified_count = 1;
  const std::string git_text = mtx::format_git_status_report(git_report);
  assert(git_text.find("Git-Status light") != std::string::npos);
  assert(git_text.find("modified=1") != std::string::npos);

  fs::remove_all(root);
  std::cout << "workspace ok\n";
}
