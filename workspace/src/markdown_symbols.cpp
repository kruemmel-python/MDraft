#include "mtx/markdown_symbols.hpp"
#include <algorithm>
#include <cctype>

namespace mtx {
namespace {

static std::string trim(std::string s) {
  auto not_space = [](unsigned char c) { return !std::isspace(c); };
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), not_space));
  s.erase(std::find_if(s.rbegin(), s.rend(), not_space).base(), s.end());
  return s;
}

static bool is_url_scheme(const std::string& s) {
  return s.find("://") != std::string::npos ||
         s.rfind("mailto:", 0) == 0 ||
         s.rfind("tel:", 0) == 0;
}

} // namespace

std::string markdown_anchor_for_heading(const std::string& title) {
  std::string out;
  out.reserve(title.size());
  bool dash = false;
  for (unsigned char c : title) {
    if (std::isalnum(c)) {
      out.push_back(static_cast<char>(std::tolower(c)));
      dash = false;
    } else if (c == ' ' || c == '-' || c == '_' || c == '\t') {
      if (!out.empty() && !dash) {
        out.push_back('-');
        dash = true;
      }
    }
  }
  while (!out.empty() && out.back() == '-') out.pop_back();
  return out.empty() ? "section" : out;
}

std::vector<MarkdownSymbol> extract_markdown_symbols(const std::string& file, const std::string& text) {
  std::vector<MarkdownSymbol> out;
  std::size_t offset = 0;
  while (offset <= text.size()) {
    std::size_t end = text.find('\n', offset);
    if (end == std::string::npos) end = text.size();
    std::string line = text.substr(offset, end - offset);
    if (!line.empty() && line.back() == '\r') line.pop_back();

    std::size_t n = 0;
    while (n < line.size() && line[n] == '#') ++n;
    if (n >= 1 && n <= 6 && n < line.size() && line[n] == ' ') {
      std::string title = trim(line.substr(n + 1));
      if (!title.empty()) {
        out.push_back(MarkdownSymbol{file, title, static_cast<int>(n), offset, markdown_anchor_for_heading(title)});
      }
    }

    if (end == text.size()) break;
    offset = end + 1;
  }
  return out;
}

std::vector<MarkdownLink> extract_markdown_links(const std::string& file, const std::string& text) {
  std::vector<MarkdownLink> out;
  for (std::size_t i = 0; i < text.size();) {
    const std::size_t lb = text.find('[', i);
    if (lb == std::string::npos) break;
    const std::size_t rb = text.find(']', lb + 1);
    if (rb == std::string::npos || rb + 1 >= text.size() || text[rb + 1] != '(') {
      i = lb + 1;
      continue;
    }
    const std::size_t rp = text.find(')', rb + 2);
    if (rp == std::string::npos) {
      i = rb + 1;
      continue;
    }
    std::string target = trim(text.substr(rb + 2, rp - rb - 2));
    const std::size_t sp = target.find_first_of(" \t");
    if (sp != std::string::npos) target = target.substr(0, sp);
    const bool is_image = lb > 0 && text[lb - 1] == '!';
    if (!target.empty() && target.front() != '#' && !is_url_scheme(target)) {
      out.push_back(MarkdownLink{file, target, lb, false, {}, is_image});
    }
    i = rp + 1;
  }
  return out;
}

} // namespace mtx
