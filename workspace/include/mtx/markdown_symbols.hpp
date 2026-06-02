#pragma once
#include "mtx/workspace.hpp"
#include <string>
#include <vector>

namespace mtx {

std::string markdown_anchor_for_heading(const std::string& title);
std::vector<MarkdownSymbol> extract_markdown_symbols(const std::string& file, const std::string& text);
std::vector<MarkdownLink> extract_markdown_links(const std::string& file, const std::string& text);

} // namespace mtx
