#pragma once
#include "mtx/workspace.hpp"
#include <cstddef>
#include <string>
#include <vector>

namespace mtx {

struct LintRuleConfig {
  std::size_t max_line_length{120};
  bool require_single_h1{true};
  bool warn_heading_level_jumps{true};
  bool warn_trailing_whitespace{true};
  bool warn_tabs{true};
  bool warn_empty_alt_text{true};
  bool warn_duplicate_anchors{true};
  bool warn_table_shape{true};
};

struct LintReport {
  std::vector<Diagnostic> diagnostics;
  std::size_t info_count{0};
  std::size_t warning_count{0};
  std::size_t error_count{0};
};

std::vector<Diagnostic> lint_markdown_document(const std::string& file,
                                               const std::string& text,
                                               const LintRuleConfig& config = {});

LintReport lint_workspace(const WorkspaceIndex& index,
                          const LintRuleConfig& config = {});

std::string format_diagnostics(const std::vector<Diagnostic>& diagnostics,
                               std::size_t max_lines = 64);

std::string format_lint_report(const LintReport& report,
                               std::size_t max_lines = 64);

} // namespace mtx
