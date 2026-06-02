#include "mtx/workspace_symbol_nav.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>

namespace mtx {
namespace {

static std::string lower_ascii(std::string s) {
  for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return s;
}

static bool contains_query(const std::string& haystack,
                           const std::string& query,
                           bool case_sensitive) {
  if (query.empty()) return true;
  if (case_sensitive) return haystack.find(query) != std::string::npos;
  return lower_ascii(haystack).find(lower_ascii(query)) != std::string::npos;
}

} // namespace

std::vector<MarkdownSymbol> find_workspace_symbols(const WorkspaceIndex& index,
                                                   const std::string& query,
                                                   SymbolQueryOptions options) {
  std::vector<MarkdownSymbol> out;
  if (options.max_results == 0) return out;

  for (const auto& symbol : index.symbols) {
    if (contains_query(symbol.title, query, options.case_sensitive) ||
        contains_query(symbol.file, query, options.case_sensitive) ||
        contains_query(symbol.anchor, query, options.case_sensitive)) {
      out.push_back(symbol);
      if (out.size() >= options.max_results) break;
    }
  }

  std::stable_sort(out.begin(), out.end(), [](const MarkdownSymbol& a, const MarkdownSymbol& b) {
    if (a.file != b.file) return a.file < b.file;
    return a.byte_offset < b.byte_offset;
  });
  return out;
}

std::string format_symbol_results(const std::vector<MarkdownSymbol>& symbols,
                                  const std::string& query,
                                  std::size_t max_lines) {
  std::ostringstream o;
  o << "Symbol-Navigation";
  if (!query.empty()) o << ": \"" << query << "\"";
  o << " | Treffer=" << symbols.size();

  const std::size_t n = std::min(max_lines, symbols.size());
  for (std::size_t i = 0; i < n; ++i) {
    const auto& s = symbols[i];
    o << "\n" << s.file << " @" << s.byte_offset << "  ";
    for (int level = 1; level < s.level; ++level) o << "  ";
    o << "# " << s.title << "  [" << s.anchor << "]";
  }
  if (symbols.size() > n) {
    o << "\n... " << (symbols.size() - n) << " weitere Symbole";
  }
  return o.str();
}

} // namespace mtx
