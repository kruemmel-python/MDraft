#include "mtx/link_validation.hpp"
#include "mtx/diagnostics.hpp"
#include <algorithm>
#include <sstream>

namespace mtx {

LinkValidationReport validate_workspace_links(const WorkspaceIndex& index) {
  LinkValidationReport r;
  for (const auto& link : index.links) {
    ++r.links_total;
    if (link.is_image) ++r.images_total;

    if (link.resolved) {
      ++r.links_resolved;
      if (link.is_image) ++r.images_resolved;
    } else {
      ++r.links_unresolved;
      if (link.is_image) ++r.images_unresolved;
    }
  }

  for (const auto& d : index.diagnostics) {
    if (d.message.find("Link nicht auflösbar:") != std::string::npos ||
        d.message.find("Bildpfad nicht auflösbar:") != std::string::npos) {
      r.diagnostics.push_back(d);
    }
  }

  std::stable_sort(r.diagnostics.begin(), r.diagnostics.end(), [](const Diagnostic& a, const Diagnostic& b) {
    if (a.file != b.file) return a.file < b.file;
    return a.byte_offset < b.byte_offset;
  });
  return r;
}

std::string format_link_validation_report(const LinkValidationReport& report,
                                          std::size_t max_lines) {
  std::ostringstream o;
  o << "Link-Validierung"
    << " | links=" << report.links_total
    << " | resolved=" << report.links_resolved
    << " | unresolved=" << report.links_unresolved
    << " | images=" << report.images_total
    << " | image_unresolved=" << report.images_unresolved;

  const std::size_t n = std::min(max_lines, report.diagnostics.size());
  for (std::size_t i = 0; i < n; ++i) {
    const auto& d = report.diagnostics[i];
    o << "\n" << d.file << " @" << d.byte_offset
      << " [" << diagnostic_severity_label(d.severity) << "] "
      << d.message;
  }
  if (report.diagnostics.size() > n) {
    o << "\n... " << (report.diagnostics.size() - n) << " weitere Diagnostics";
  }
  return o.str();
}

} // namespace mtx
