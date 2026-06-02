#pragma once
#include "mtx/workspace.hpp"
#include <cstddef>
#include <string>
#include <vector>

namespace mtx {

struct ImageSuggestion {
  std::string path;
  int score{0};
  std::string reason;
};

struct ImageSuggestionOptions {
  std::size_t max_results{24};
  bool prefer_same_directory{true};
};

std::vector<ImageSuggestion> suggest_workspace_images(const WorkspaceIndex& index,
                                                      const std::string& current_file,
                                                      const std::string& query,
                                                      ImageSuggestionOptions options = {});
std::string format_image_suggestions(const std::vector<ImageSuggestion>& suggestions,
                                     const std::string& query,
                                     std::size_t max_lines = 24);

} // namespace mtx
