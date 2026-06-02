#pragma once
#include "mtx/workspace.hpp"
#include <cstddef>
#include <string>
#include <vector>

namespace mtx {

struct LinkValidationReport {
  std::size_t links_total{0};
  std::size_t links_resolved{0};
  std::size_t links_unresolved{0};
  std::size_t images_total{0};
  std::size_t images_resolved{0};
  std::size_t images_unresolved{0};
  std::vector<Diagnostic> diagnostics;
};

LinkValidationReport validate_workspace_links(const WorkspaceIndex& index);
std::string format_link_validation_report(const LinkValidationReport& report,
                                          std::size_t max_lines = 32);

} // namespace mtx
