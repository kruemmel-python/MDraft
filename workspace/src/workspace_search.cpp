#include "mtx/workspace_search.hpp"
#include "mtx/file_io.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>

namespace mtx {
namespace {

static std::string lower_ascii(std::string s) {
  for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return s;
}

static std::pair<std::size_t, std::size_t> line_col_for_offset(const std::string& text, std::size_t offset) {
  std::size_t line = 1;
  std::size_t col = 1;
  offset = std::min(offset, text.size());
  for (std::size_t i = 0; i < offset; ++i) {
    if (text[i] == '\n') {
      ++line;
      col = 1;
    } else {
      ++col;
    }
  }
  return {line, col};
}

static std::string snippet_around(const std::string& text, std::size_t pos, std::size_t len, std::size_t context) {
  const std::size_t begin = pos > context ? pos - context : 0;
  const std::size_t end = std::min(text.size(), pos + len + context);
  std::string s = text.substr(begin, end - begin);
  for (char& c : s) {
    if (c == '\r' || c == '\n' || c == '\t') c = ' ';
  }
  if (begin > 0) s = "…" + s;
  if (end < text.size()) s += "…";
  return s;
}

} // namespace

std::vector<SearchResult> search_workspace(const WorkspaceIndex& index,
                                           const std::string& query,
                                           SearchOptions options) {
  std::vector<SearchResult> out;
  if (query.empty() || index.files.empty() || options.max_results == 0) return out;

  const std::string needle = options.case_sensitive ? query : lower_ascii(query);
  for (const auto& file : index.files) {
    if (out.size() >= options.max_results) break;
    const std::string text = read_file(file.path);
    const std::string hay = options.case_sensitive ? text : lower_ascii(text);
    std::size_t pos = 0;
    while (pos <= hay.size() && out.size() < options.max_results) {
      const std::size_t hit = hay.find(needle, pos);
      if (hit == std::string::npos) break;
      const auto lc = line_col_for_offset(text, hit);
      out.push_back(SearchResult{
        file.relative_path,
        lc.first,
        lc.second,
        hit,
        snippet_around(text, hit, query.size(), options.context_chars)
      });
      pos = hit + std::max<std::size_t>(1, needle.size());
    }
  }
  return out;
}

std::string format_search_results(const std::vector<SearchResult>& results,
                                  const std::string& query,
                                  std::size_t max_lines) {
  std::ostringstream o;
  o << "Suche: \"" << query << "\" | Treffer=" << results.size();
  const std::size_t n = std::min(max_lines, results.size());
  for (std::size_t i = 0; i < n; ++i) {
    const auto& r = results[i];
    o << "\n" << r.file << ":" << r.line << ":" << r.column << "  " << r.snippet;
  }
  if (results.size() > n) {
    o << "\n… " << (results.size() - n) << " weitere Treffer";
  }
  return o.str();
}

} // namespace mtx
