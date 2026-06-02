#pragma once
#include "mtx/workspace.hpp"
#include <string>

namespace mtx {

bool workspace_path_is_markdown(const std::string& path);
bool workspace_path_is_image(const std::string& path);
WorkspaceIndex build_workspace_index(const std::string& root_path);

} // namespace mtx
