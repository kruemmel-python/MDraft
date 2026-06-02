#pragma once
#include <cstddef>
#include <string>
#include <vector>

namespace mtx {

struct Snippet {
  std::string trigger;
  std::string label;
  std::string body;
  std::string description;
};

struct SnippetExpansion {
  bool matched{false};
  std::string trigger;
  std::string body;
  std::size_t cursor_offset{0};
};

const std::vector<Snippet>& default_snippets();
SnippetExpansion expand_snippet_trigger(const std::string& trigger);
std::string format_snippet_catalog();
std::string make_markdown_image(const std::string& alt, const std::string& href);

} // namespace mtx
