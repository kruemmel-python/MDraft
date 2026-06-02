#pragma once
#include "mtx/workspace.hpp"
#include <cstddef>
#include <string>
#include <vector>

namespace mtx {

struct SearchOptions {
  bool case_sensitive{false};
  std::size_t max_results{256};
  std::size_t context_chars{96};
};

struct SearchResult {
  std::string file;
  std::size_t line{0};
  std::size_t column{0};
  std::size_t byte_offset{0};
  std::string snippet;
};

std::vector<SearchResult> search_workspace(const WorkspaceIndex& index,
                                           const std::string& query,
                                           SearchOptions options = {});
std::string format_search_results(const std::vector<SearchResult>& results,
                                  const std::string& query,
                                  std::size_t max_lines = 12);

} // namespace mtx
