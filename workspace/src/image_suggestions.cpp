#include "mtx/image_suggestions.hpp"
#include "mtx/file_index.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>

namespace mtx {
namespace {

static std::string lower_ascii(std::string s) {
  for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return s;
}

static std::string basename_of(const std::string& path) {
  const std::size_t slash = path.find_last_of('/');
  return slash == std::string::npos ? path : path.substr(slash + 1);
}

static std::string parent_of(const std::string& path) {
  const std::size_t slash = path.find_last_of('/');
  return slash == std::string::npos ? std::string{} : path.substr(0, slash);
}

static std::string extension_of(const std::string& path) {
  const std::string base = basename_of(path);
  const std::size_t dot = base.find_last_of('.');
  return dot == std::string::npos ? std::string{} : lower_ascii(base.substr(dot));
}

static bool contains(const std::string& haystack, const std::string& needle) {
  if (needle.empty()) return true;
  return lower_ascii(haystack).find(lower_ascii(needle)) != std::string::npos;
}

} // namespace

std::vector<ImageSuggestion> suggest_workspace_images(const WorkspaceIndex& index,
                                                      const std::string& current_file,
                                                      const std::string& query,
                                                      ImageSuggestionOptions options) {
  std::vector<ImageSuggestion> out;
  if (options.max_results == 0) return out;

  const std::string q = lower_ascii(query);
  const std::string current_parent = parent_of(current_file);

  for (const auto& asset : index.assets) {
    if (!workspace_path_is_image(asset.relative_path)) continue;

    int score = 10;
    std::string reason = "Bild im Workspace";

    const std::string base = basename_of(asset.relative_path);
    const std::string ext = extension_of(asset.relative_path);
    if (!q.empty()) {
      if (contains(base, q)) {
        score += 100;
        reason = "Dateiname passt";
      } else if (contains(asset.relative_path, q)) {
        score += 60;
        reason = "Pfad passt";
      } else {
        continue;
      }
    }

    if (options.prefer_same_directory && !current_parent.empty() &&
        parent_of(asset.relative_path) == current_parent) {
      score += 35;
      reason += ", gleicher Ordner";
    }

    if (ext == ".svg") score += 8;
    else if (ext == ".png" || ext == ".webp") score += 6;
    else if (ext == ".jpg" || ext == ".jpeg") score += 4;

    out.push_back(ImageSuggestion{asset.relative_path, score, reason});
  }

  std::stable_sort(out.begin(), out.end(), [](const ImageSuggestion& a, const ImageSuggestion& b) {
    if (a.score != b.score) return a.score > b.score;
    return a.path < b.path;
  });
  if (out.size() > options.max_results) out.resize(options.max_results);
  return out;
}

std::string format_image_suggestions(const std::vector<ImageSuggestion>& suggestions,
                                     const std::string& query,
                                     std::size_t max_lines) {
  std::ostringstream o;
  o << "Bildpfad-Vorschläge";
  if (!query.empty()) o << ": \"" << query << "\"";
  o << " | Treffer=" << suggestions.size();

  const std::size_t n = std::min(max_lines, suggestions.size());
  for (std::size_t i = 0; i < n; ++i) {
    const auto& s = suggestions[i];
    o << "\n" << s.path << "  score=" << s.score << "  " << s.reason;
  }
  if (suggestions.size() > n) {
    o << "\n... " << (suggestions.size() - n) << " weitere Bilder";
  }
  return o.str();
}

} // namespace mtx
