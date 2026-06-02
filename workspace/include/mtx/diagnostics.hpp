#pragma once
#include "mtx/workspace.hpp"
#include <string>

namespace mtx {

std::string diagnostic_severity_label(DiagnosticSeverity s);
std::string workspace_summary(const WorkspaceIndex& index);

} // namespace mtx
