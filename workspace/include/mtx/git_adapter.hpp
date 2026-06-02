#pragma once
#include <cstddef>
#include <string>
#include <vector>

namespace mtx {

enum class GitChangeKind {
  Modified,
  Added,
  Deleted,
  Renamed,
  Copied,
  Untracked,
  Ignored,
  Conflict,
  Unknown
};

struct GitFileStatus {
  GitChangeKind kind{GitChangeKind::Unknown};
  std::string index_status;
  std::string worktree_status;
  std::string path;
  std::string old_path;
};

struct GitStatusReport {
  bool git_available{false};
  bool repository{false};
  std::string root;
  std::vector<GitFileStatus> files;
  std::string error;

  std::size_t modified_count{0};
  std::size_t added_count{0};
  std::size_t deleted_count{0};
  std::size_t renamed_count{0};
  std::size_t untracked_count{0};
  std::size_t conflict_count{0};
};

const char* git_change_kind_label(GitChangeKind kind) noexcept;

GitFileStatus parse_git_status_porcelain_line(const std::string& line);
std::string find_git_root(const std::string& path_hint);
GitStatusReport read_git_status(const std::string& path_hint);
std::string format_git_status_report(const GitStatusReport& report,
                                     std::size_t max_lines = 80);

} // namespace mtx
