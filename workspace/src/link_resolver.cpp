#include "mtx/link_resolver.hpp"
#include <algorithm>
#include <filesystem>
#include <set>

namespace mtx {
namespace fs = std::filesystem;

namespace {

static std::string normalize_rel(std::string s) {
  std::replace(s.begin(), s.end(), '\\', '/');
  const std::size_t hash = s.find('#');
  if (hash != std::string::npos) s = s.substr(0, hash);
  return normalize_workspace_path(s);
}

static std::string parent_rel(const std::string& rel) {
  const std::size_t slash = rel.find_last_of('/');
  return slash == std::string::npos ? std::string{} : rel.substr(0, slash);
}

static std::string join_rel(const std::string& parent, const std::string& target) {
  fs::path p(parent.empty() ? fs::path(target) : fs::path(parent) / fs::path(target));
  return normalize_workspace_path(p.lexically_normal().generic_string());
}

} // namespace

void resolve_workspace_links(WorkspaceIndex& index) {
  std::set<std::string> files;
  for (const auto& f : index.files) files.insert(normalize_rel(f.relative_path));
  for (const auto& a : index.assets) files.insert(normalize_rel(a.relative_path));

  for (auto& link : index.links) {
    const std::string target = normalize_rel(link.target);
    if (target.empty()) continue;
    const std::string candidate = join_rel(parent_rel(link.file), target);
    if (files.find(candidate) != files.end()) {
      link.resolved = true;
      link.resolved_path = candidate;
    } else {
      link.resolved = false;
      link.resolved_path.clear();
      index.diagnostics.push_back(Diagnostic{link.file, link.byte_offset, DiagnosticSeverity::Warning,
        std::string(link.is_image ? "Bildpfad nicht auflösbar: " : "Link nicht auflösbar: ") + link.target});
    }
  }
}

} // namespace mtx
