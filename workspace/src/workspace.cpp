#include "mtx/workspace.hpp"
#include <algorithm>

namespace mtx {

void WorkspaceIndex::clear() {
  root.path.clear();
  files.clear();
  assets.clear();
  symbols.clear();
  links.clear();
  diagnostics.clear();
}

std::string normalize_workspace_path(const std::string& path) {
  std::string out = path;
  std::replace(out.begin(), out.end(), '\\', '/');
  while (out.size() > 1 && out.back() == '/') out.pop_back();
  return out;
}

std::uint64_t fnv1a64(const std::string& text) noexcept {
  std::uint64_t h = 1469598103934665603ull;
  for (unsigned char c : text) {
    h ^= static_cast<std::uint64_t>(c);
    h *= 1099511628211ull;
  }
  return h;
}

} // namespace mtx
