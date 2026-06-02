#include "mtx/file_index.hpp"
#include "mtx/file_io.hpp"
#include "mtx/markdown_symbols.hpp"
#include "mtx/link_resolver.hpp"
#include <algorithm>
#include <chrono>
#include <cctype>
#include <filesystem>
#include <system_error>

namespace mtx {
namespace fs = std::filesystem;

namespace {

static std::string to_generic(const fs::path& p) {
  return p.generic_string();
}

static bool ignored_dir(const fs::path& p) {
  const std::string n = p.filename().generic_string();
  return n == ".git" || n == ".vs" || n == "build" || n == "build_vs" ||
         n == "build_vs_msi" || n == "build_ninja" || n == "node_modules";
}

static std::uint64_t modified_ticks(const fs::path& p) {
  std::error_code ec;
  const auto ft = fs::last_write_time(p, ec);
  if (ec) return 0;
  return static_cast<std::uint64_t>(ft.time_since_epoch().count());
}

} // namespace

bool workspace_path_is_markdown(const std::string& path) {
  std::string p = path;
  std::replace(p.begin(), p.end(), '\\', '/');
  const std::size_t dot = p.find_last_of('.');
  if (dot == std::string::npos) return false;
  std::string ext = p.substr(dot);
  std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
  return ext == ".md" || ext == ".markdown" || ext == ".mdown" || ext == ".txt";
}

bool workspace_path_is_image(const std::string& path) {
  std::string p = path;
  std::replace(p.begin(), p.end(), '\\', '/');
  const std::size_t dot = p.find_last_of('.');
  if (dot == std::string::npos) return false;
  std::string ext = p.substr(dot);
  std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
  return ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".gif" ||
         ext == ".webp" || ext == ".bmp" || ext == ".svg";
}

WorkspaceIndex build_workspace_index(const std::string& root_path) {
  WorkspaceIndex index;
  const fs::path root = fs::absolute(fs::path(root_path));
  index.root.path = normalize_workspace_path(to_generic(root));

  std::error_code ec;
  if (!fs::exists(root, ec) || !fs::is_directory(root, ec)) {
    index.diagnostics.push_back(Diagnostic{"", 0, DiagnosticSeverity::Error, "Workspace root existiert nicht: " + index.root.path});
    return index;
  }

  fs::recursive_directory_iterator it(root, fs::directory_options::skip_permission_denied, ec);
  fs::recursive_directory_iterator end;
  for (; it != end; it.increment(ec)) {
    if (ec) {
      index.diagnostics.push_back(Diagnostic{"", 0, DiagnosticSeverity::Warning, "Index-Scan übersprungen: " + ec.message()});
      ec.clear();
      continue;
    }
    const fs::path p = it->path();
    if (it->is_directory(ec)) {
      if (ignored_dir(p)) it.disable_recursion_pending();
      continue;
    }
    if (!it->is_regular_file(ec)) continue;
    const std::string path = normalize_workspace_path(to_generic(p));
    const std::string rel = normalize_workspace_path(to_generic(fs::relative(p, root, ec)));

    if (!workspace_path_is_markdown(path)) {
      WorkspaceAsset a;
      a.path = path;
      a.relative_path = ec ? path : rel;
      std::error_code sec;
      a.size = static_cast<std::uint64_t>(fs::file_size(p, sec));
      if (sec) a.size = 0;
      a.modified_ticks = modified_ticks(p);
      index.assets.push_back(a);
      continue;
    }

    const std::string text = read_file(path);
    WorkspaceFile f;
    f.path = path;
    f.relative_path = ec ? path : rel;
    f.size = static_cast<std::uint64_t>(text.size());
    f.modified_ticks = modified_ticks(p);
    f.content_hash = fnv1a64(text);
    index.files.push_back(f);

    auto symbols = extract_markdown_symbols(f.relative_path, text);
    index.symbols.insert(index.symbols.end(), symbols.begin(), symbols.end());
    auto links = extract_markdown_links(f.relative_path, text);
    index.links.insert(index.links.end(), links.begin(), links.end());
  }

  std::sort(index.files.begin(), index.files.end(), [](const WorkspaceFile& a, const WorkspaceFile& b) {
    return a.relative_path < b.relative_path;
  });
  std::sort(index.assets.begin(), index.assets.end(), [](const WorkspaceAsset& a, const WorkspaceAsset& b) {
    return a.relative_path < b.relative_path;
  });
  resolve_workspace_links(index);
  return index;
}

} // namespace mtx
