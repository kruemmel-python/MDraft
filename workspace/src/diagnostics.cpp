#include "mtx/diagnostics.hpp"
#include <sstream>

namespace mtx {

std::string diagnostic_severity_label(DiagnosticSeverity s) {
  switch (s) {
    case DiagnosticSeverity::Info: return "info";
    case DiagnosticSeverity::Warning: return "warning";
    case DiagnosticSeverity::Error: return "error";
    default: return "unknown";
  }
}

std::string workspace_summary(const WorkspaceIndex& index) {
  std::ostringstream o;
  o << "Workspace: " << (index.root.path.empty() ? "<none>" : index.root.path)
    << " | files=" << index.files.size()
    << " | assets=" << index.assets.size()
    << " | symbols=" << index.symbols.size()
    << " | links=" << index.links.size()
    << " | diagnostics=" << index.diagnostics.size();
  return o.str();
}

} // namespace mtx
