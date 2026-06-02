#include "mtx/git_adapter.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <sstream>
#include <string>

#ifdef _WIN32
#define MTX_POPEN _popen
#define MTX_PCLOSE _pclose
#else
#define MTX_POPEN popen
#define MTX_PCLOSE pclose
#endif

namespace mtx {
namespace {

static std::string trim_copy(std::string s) {
  auto not_space = [](unsigned char c) { return !std::isspace(c); };
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), not_space));
  s.erase(std::find_if(s.rbegin(), s.rend(), not_space).base(), s.end());
  return s;
}

static std::string shell_quote(const std::string& s) {
#ifdef _WIN32
  std::string out = "\"";
  for (char c : s) {
    if (c == '"') out += "\\\"";
    else out.push_back(c);
  }
  out += "\"";
  return out;
#else
  std::string out = "'";
  for (char c : s) {
    if (c == '\'') out += "'\\''";
    else out.push_back(c);
  }
  out += "'";
  return out;
#endif
}

static std::string redirect_stderr_null() {
#ifdef _WIN32
  return " 2>NUL";
#else
  return " 2>/dev/null";
#endif
}

static std::string run_capture(const std::string& command, int* exit_code = nullptr) {
  if (exit_code) *exit_code = -1;
  FILE* pipe = MTX_POPEN(command.c_str(), "r");
  if (!pipe) return {};

  std::string out;
  std::array<char, 4096> buffer{};
  while (std::fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
    out += buffer.data();
  }
  const int rc = MTX_PCLOSE(pipe);
  if (exit_code) *exit_code = rc;
  return out;
}

static std::vector<std::string> split_lines(const std::string& s) {
  std::vector<std::string> lines;
  std::size_t p = 0;
  while (p <= s.size()) {
    std::size_t e = s.find('\n', p);
    if (e == std::string::npos) e = s.size();
    std::string line = s.substr(p, e - p);
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (!line.empty()) lines.push_back(line);
    if (e == s.size()) break;
    p = e + 1;
  }
  return lines;
}

static GitChangeKind classify_status(char x, char y) {
  if (x == '?' && y == '?') return GitChangeKind::Untracked;
  if (x == '!' && y == '!') return GitChangeKind::Ignored;
  if (x == 'U' || y == 'U' || (x == 'A' && y == 'A') || (x == 'D' && y == 'D')) return GitChangeKind::Conflict;
  if (x == 'R' || y == 'R') return GitChangeKind::Renamed;
  if (x == 'C' || y == 'C') return GitChangeKind::Copied;
  if (x == 'A' || y == 'A') return GitChangeKind::Added;
  if (x == 'D' || y == 'D') return GitChangeKind::Deleted;
  if (x == 'M' || y == 'M') return GitChangeKind::Modified;
  return GitChangeKind::Unknown;
}

static void accumulate(GitStatusReport& report, GitChangeKind kind) {
  switch (kind) {
    case GitChangeKind::Modified: ++report.modified_count; break;
    case GitChangeKind::Added: ++report.added_count; break;
    case GitChangeKind::Deleted: ++report.deleted_count; break;
    case GitChangeKind::Renamed:
    case GitChangeKind::Copied: ++report.renamed_count; break;
    case GitChangeKind::Untracked: ++report.untracked_count; break;
    case GitChangeKind::Conflict: ++report.conflict_count; break;
    default: break;
  }
}

} // namespace

const char* git_change_kind_label(GitChangeKind kind) noexcept {
  switch (kind) {
    case GitChangeKind::Modified: return "modified";
    case GitChangeKind::Added: return "added";
    case GitChangeKind::Deleted: return "deleted";
    case GitChangeKind::Renamed: return "renamed";
    case GitChangeKind::Copied: return "copied";
    case GitChangeKind::Untracked: return "untracked";
    case GitChangeKind::Ignored: return "ignored";
    case GitChangeKind::Conflict: return "conflict";
    case GitChangeKind::Unknown:
    default: return "unknown";
  }
}

GitFileStatus parse_git_status_porcelain_line(const std::string& line) {
  GitFileStatus status;
  if (line.size() < 3) return status;

  const char x = line[0];
  const char y = line[1];
  status.index_status = std::string(1, x);
  status.worktree_status = std::string(1, y);
  status.kind = classify_status(x, y);

  std::string path = trim_copy(line.substr(3));
  const std::string marker = " -> ";
  const std::size_t rename = path.find(marker);
  if (rename != std::string::npos) {
    status.old_path = path.substr(0, rename);
    status.path = path.substr(rename + marker.size());
  } else {
    status.path = path;
  }

  if (status.path.size() >= 2 && status.path.front() == '"' && status.path.back() == '"') {
    status.path = status.path.substr(1, status.path.size() - 2);
  }
  if (status.old_path.size() >= 2 && status.old_path.front() == '"' && status.old_path.back() == '"') {
    status.old_path = status.old_path.substr(1, status.old_path.size() - 2);
  }
  return status;
}

std::string find_git_root(const std::string& path_hint) {
  const std::string hint = path_hint.empty() ? "." : path_hint;
  const std::string command =
    "git -C " + shell_quote(hint) + " rev-parse --show-toplevel" + redirect_stderr_null();
  int rc = -1;
  std::string out = run_capture(command, &rc);
  out = trim_copy(out);
  return out;
}

GitStatusReport read_git_status(const std::string& path_hint) {
  GitStatusReport report;
  const std::string root = find_git_root(path_hint);
  if (root.empty()) {
    report.git_available = false;
    report.repository = false;
    report.error = "Kein Git-Repository gefunden oder git.exe nicht im PATH.";
    return report;
  }

  report.git_available = true;
  report.repository = true;
  report.root = root;

  const std::string command =
    "git -C " + shell_quote(root) + " status --porcelain=v1" + redirect_stderr_null();
  int rc = -1;
  const std::string out = run_capture(command, &rc);
  const std::vector<std::string> lines = split_lines(out);
  for (const std::string& line : lines) {
    GitFileStatus st = parse_git_status_porcelain_line(line);
    if (st.path.empty()) continue;
    accumulate(report, st.kind);
    report.files.push_back(std::move(st));
  }
  return report;
}

std::string format_git_status_report(const GitStatusReport& report, std::size_t max_lines) {
  std::ostringstream o;
  o << "Git-Status light\n\n";
  if (!report.repository) {
    o << report.error << "\n";
    return o.str();
  }

  o << "Root: " << report.root << "\n";
  o << "Files: " << report.files.size()
    << "  modified=" << report.modified_count
    << "  added=" << report.added_count
    << "  deleted=" << report.deleted_count
    << "  renamed=" << report.renamed_count
    << "  untracked=" << report.untracked_count
    << "  conflicts=" << report.conflict_count << "\n\n";

  if (report.files.empty()) {
    o << "Working tree clean.\n";
    return o.str();
  }

  const std::size_t n = std::min(max_lines, report.files.size());
  for (std::size_t i = 0; i < n; ++i) {
    const GitFileStatus& f = report.files[i];
    o << "[" << git_change_kind_label(f.kind) << "] "
      << f.index_status << f.worktree_status << " ";
    if (!f.old_path.empty()) o << f.old_path << " -> ";
    o << f.path << "\n";
  }
  if (report.files.size() > n) {
    o << "... +" << (report.files.size() - n) << " weitere Dateien\n";
  }
  return o.str();
}

} // namespace mtx
