#pragma once
#include "mtx/workspace.hpp"
#include <cstddef>
#include <string>
#include <vector>

namespace mtx {

struct SymbolQueryOptions {
  bool case_sensitive{false};
  std::size_t max_results{64};
};

std::vector<MarkdownSymbol> find_workspace_symbols(const WorkspaceIndex& index,
                                                   const std::string& query,
                                                   SymbolQueryOptions options = {});

std::string format_symbol_results(const std::vector<MarkdownSymbol>& symbols,
                                  const std::string& query,
                                  std::size_t max_lines = 32);

} // namespace mtx
