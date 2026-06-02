#pragma once
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace mtx {

struct WorkspaceRoot {
  std::string path;
};

struct WorkspaceFile {
  std::string path;
  std::string relative_path;
  std::uint64_t size{0};
  std::uint64_t modified_ticks{0};
  std::uint64_t content_hash{0};
};

struct WorkspaceAsset {
  std::string path;
  std::string relative_path;
  std::uint64_t size{0};
  std::uint64_t modified_ticks{0};
};

struct MarkdownSymbol {
  std::string file;
  std::string title;
  int level{0};
  std::size_t byte_offset{0};
  std::string anchor;
};

struct MarkdownLink {
  std::string file;
  std::string target;
  std::size_t byte_offset{0};
  bool resolved{false};
  std::string resolved_path;
  bool is_image{false};
};

enum class DiagnosticSeverity {
  Info,
  Warning,
  Error
};

struct Diagnostic {
  std::string file;
  std::size_t byte_offset{0};
  DiagnosticSeverity severity{DiagnosticSeverity::Info};
  std::string message;
  std::size_t line{0};
  std::size_t column{0};
  std::string code;
  std::string fix_hint;

  Diagnostic() = default;
  Diagnostic(std::string file_,
             std::size_t byte_offset_,
             DiagnosticSeverity severity_,
             std::string message_,
             std::size_t line_ = 0,
             std::size_t column_ = 0,
             std::string code_ = {},
             std::string fix_hint_ = {})
    : file(std::move(file_)),
      byte_offset(byte_offset_),
      severity(severity_),
      message(std::move(message_)),
      line(line_),
      column(column_),
      code(std::move(code_)),
      fix_hint(std::move(fix_hint_)) {}
};

struct WorkspaceIndex {
  WorkspaceRoot root;
  std::vector<WorkspaceFile> files;
  std::vector<WorkspaceAsset> assets;
  std::vector<MarkdownSymbol> symbols;
  std::vector<MarkdownLink> links;
  std::vector<Diagnostic> diagnostics;

  void clear();
  bool empty() const noexcept { return root.path.empty(); }
};

std::string normalize_workspace_path(const std::string& path);
std::uint64_t fnv1a64(const std::string& text) noexcept;

} // namespace mtx
