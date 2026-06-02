#include "mtx/workspace_lint.hpp"
#include "mtx/diagnostics.hpp"
#include "mtx/file_io.hpp"
#include <algorithm>
#include <cctype>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace mtx {
namespace {

struct LineInfo {
  std::string text;
  std::size_t byte_offset{0};
};

static std::string trim_copy(std::string s) {
  auto not_space = [](unsigned char c) { return !std::isspace(c); };
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), not_space));
  s.erase(std::find_if(s.rbegin(), s.rend(), not_space).base(), s.end());
  return s;
}


static std::vector<LineInfo> split_lines_with_offsets(const std::string& text) {
  std::vector<LineInfo> out;
  std::size_t pos = 0;
  while (pos <= text.size()) {
    const std::size_t begin = pos;
    std::size_t end = text.find('\n', pos);
    if (end == std::string::npos) end = text.size();
    std::string line = text.substr(begin, end - begin);
    if (!line.empty() && line.back() == '\r') line.pop_back();
    out.push_back(LineInfo{line, begin});
    if (end == text.size()) break;
    pos = end + 1;
  }
  return out;
}

static std::string make_anchor(std::string title) {
  std::string out;
  bool dash = false;
  for (unsigned char ch : title) {
    if (std::isalnum(ch)) {
      out.push_back(static_cast<char>(std::tolower(ch)));
      dash = false;
    } else if (std::isspace(ch) || ch == '-' || ch == '_') {
      if (!dash && !out.empty()) {
        out.push_back('-');
        dash = true;
      }
    }
  }
  while (!out.empty() && out.back() == '-') out.pop_back();
  return out;
}

static int heading_level(const std::string& t) {
  int n = 0;
  while (n < static_cast<int>(t.size()) && t[static_cast<std::size_t>(n)] == '#') ++n;
  if (n >= 1 && n <= 6 && n < static_cast<int>(t.size()) && t[static_cast<std::size_t>(n)] == ' ') return n;
  return 0;
}

static std::size_t count_table_cells(const std::string& line) {
  const std::string t = trim_copy(line);
  if (t.empty() || t.find('|') == std::string::npos) return 0;
  std::size_t cells = 1;
  for (char c : t) if (c == '|') ++cells;
  if (!t.empty() && t.front() == '|') --cells;
  if (!t.empty() && t.back() == '|') --cells;
  return cells;
}

static bool table_separator_like(const std::string& line) {
  const std::string t = trim_copy(line);
  if (t.find('|') == std::string::npos || t.find('-') == std::string::npos) return false;
  for (char c : t) {
    if (c != '|' && c != '-' && c != ':' && !std::isspace(static_cast<unsigned char>(c))) return false;
  }
  return true;
}

static void add_diag(std::vector<Diagnostic>& out,
                     const std::string& file,
                     std::size_t byte_offset,
                     DiagnosticSeverity severity,
                     const std::string& code,
                     const std::string& message,
                     std::size_t line,
                     std::size_t column,
                     const std::string& hint = {}) {
  Diagnostic d;
  d.file = file;
  d.byte_offset = byte_offset;
  d.severity = severity;
  d.message = message;
  d.line = line;
  d.column = column;
  d.code = code;
  d.fix_hint = hint;
  out.push_back(std::move(d));
}

} // namespace

std::vector<Diagnostic> lint_markdown_document(const std::string& file,
                                               const std::string& text,
                                               const LintRuleConfig& config) {
  std::vector<Diagnostic> out;
  const std::vector<LineInfo> lines = split_lines_with_offsets(text);
  std::map<std::string, std::size_t> anchor_counts;
  int last_heading_level = 0;
  std::size_t h1_count = 0;
  std::size_t first_heading_line = 0;

  for (std::size_t i = 0; i < lines.size(); ++i) {
    const std::string& line = lines[i].text;
    const std::size_t line_no = i + 1;

    if (config.warn_trailing_whitespace && !line.empty() &&
        (line.back() == ' ' || line.back() == '\t')) {
      add_diag(out, file, lines[i].byte_offset + line.size() - 1, DiagnosticSeverity::Warning,
               "MD001", "Trailing whitespace", line_no, line.size(),
               "Zeilenende ohne Leerzeichen speichern.");
    }

    if (config.warn_tabs) {
      const std::size_t tab = line.find('\t');
      if (tab != std::string::npos) {
        add_diag(out, file, lines[i].byte_offset + tab, DiagnosticSeverity::Warning,
                 "MD002", "Tabulator im Markdown-Text", line_no, tab + 1,
                 "Tabs durch Spaces ersetzen.");
      }
    }

    if (config.max_line_length > 0 && line.size() > config.max_line_length) {
      add_diag(out, file, lines[i].byte_offset + config.max_line_length, DiagnosticSeverity::Info,
               "MD003", "Sehr lange Zeile: " + std::to_string(line.size()) + " Zeichen",
               line_no, config.max_line_length + 1,
               "Optional umbrechen, wenn die Lesbarkeit leidet.");
    }

    const int level = heading_level(trim_copy(line));
    if (level > 0) {
      if (first_heading_line == 0) first_heading_line = line_no;
      if (level == 1) ++h1_count;
      if (config.warn_heading_level_jumps && last_heading_level > 0 && level > last_heading_level + 1) {
        add_diag(out, file, lines[i].byte_offset, DiagnosticSeverity::Warning,
                 "MD004", "Heading-Level springt von H" + std::to_string(last_heading_level) +
                          " auf H" + std::to_string(level),
                 line_no, 1, "Zwischenüberschrift ergänzen oder Level korrigieren.");
      }
      last_heading_level = level;
      const std::string title = trim_copy(line.substr(static_cast<std::size_t>(level)));
      const std::string anchor = make_anchor(title);
      if (!anchor.empty()) {
        ++anchor_counts[anchor];
        if (config.warn_duplicate_anchors && anchor_counts[anchor] > 1) {
          add_diag(out, file, lines[i].byte_offset, DiagnosticSeverity::Warning,
                   "MD005", "Doppelter Heading-Anchor: #" + anchor,
                   line_no, 1, "Heading-Titel eindeutig machen.");
        }
      }
    }

    if (config.warn_empty_alt_text) {
      std::size_t pos = line.find("![](");
      if (pos != std::string::npos) {
        add_diag(out, file, lines[i].byte_offset + pos, DiagnosticSeverity::Info,
                 "MD006", "Bild ohne Alt-Text", line_no, pos + 1,
                 "Alt-Text für Barrierefreiheit ergänzen.");
      }
    }

    if (line.find("[](") != std::string::npos && line.find("![](") == std::string::npos) {
      const std::size_t pos = line.find("[](");
      add_diag(out, file, lines[i].byte_offset + pos, DiagnosticSeverity::Warning,
               "MD007", "Link ohne Linktext", line_no, pos + 1,
               "Sichtbaren Linktext ergänzen.");
    }

    if (config.warn_table_shape && i + 1 < lines.size() && table_separator_like(lines[i + 1].text)) {
      const std::size_t header_cells = count_table_cells(line);
      const std::size_t sep_cells = count_table_cells(lines[i + 1].text);
      if (header_cells != sep_cells) {
        add_diag(out, file, lines[i + 1].byte_offset, DiagnosticSeverity::Warning,
                 "MD008", "Tabellenkopf und Separator haben unterschiedliche Spaltenzahl",
                 i + 2, 1, "Pipe-Spalten angleichen.");
      }
    }
  }

  if (config.require_single_h1 && !lines.empty()) {
    if (h1_count == 0) {
      add_diag(out, file, 0, DiagnosticSeverity::Info,
               "MD009", "Dokument hat keine H1-Überschrift", 1, 1,
               "Mit '# Titel' beginnen.");
    } else if (h1_count > 1) {
      add_diag(out, file, 0, DiagnosticSeverity::Warning,
               "MD010", "Dokument hat mehrere H1-Überschriften", first_heading_line, 1,
               "Nur eine H1 pro Dokument verwenden.");
    }
  }

  return out;
}

LintReport lint_workspace(const WorkspaceIndex& index, const LintRuleConfig& config) {
  LintReport report;
  report.diagnostics = index.diagnostics;

  for (const WorkspaceFile& file : index.files) {
    const std::string text = read_file(file.path);
    auto d = lint_markdown_document(file.relative_path, text, config);
    report.diagnostics.insert(report.diagnostics.end(), d.begin(), d.end());
  }

  std::sort(report.diagnostics.begin(), report.diagnostics.end(), [](const Diagnostic& a, const Diagnostic& b) {
    if (a.severity != b.severity) return static_cast<int>(a.severity) > static_cast<int>(b.severity);
    if (a.file != b.file) return a.file < b.file;
    return a.byte_offset < b.byte_offset;
  });

  for (const Diagnostic& d : report.diagnostics) {
    switch (d.severity) {
      case DiagnosticSeverity::Error: ++report.error_count; break;
      case DiagnosticSeverity::Warning: ++report.warning_count; break;
      case DiagnosticSeverity::Info: ++report.info_count; break;
    }
  }
  return report;
}

std::string format_diagnostics(const std::vector<Diagnostic>& diagnostics,
                               std::size_t max_lines) {
  std::ostringstream o;
  std::size_t n = 0;
  for (const Diagnostic& d : diagnostics) {
    if (n++ >= max_lines) {
      o << "... " << (diagnostics.size() - max_lines) << " weitere Diagnostics\n";
      break;
    }
    o << diagnostic_severity_label(d.severity);
    if (!d.code.empty()) o << " " << d.code;
    o << " " << (d.file.empty() ? "<workspace>" : d.file);
    if (d.line != 0) o << ":" << d.line << ":" << (d.column == 0 ? 1 : d.column);
    else if (d.byte_offset != 0) o << "@byte " << d.byte_offset;
    o << " - " << d.message;
    if (!d.fix_hint.empty()) o << " [" << d.fix_hint << "]";
    o << "\n";
  }
  if (diagnostics.empty()) o << "Keine Diagnostics.\n";
  return o.str();
}

std::string format_lint_report(const LintReport& report,
                               std::size_t max_lines) {
  std::ostringstream o;
  o << "Diagnostics/Linting\n"
    << "  errors=" << report.error_count
    << " warnings=" << report.warning_count
    << " infos=" << report.info_count
    << " total=" << report.diagnostics.size() << "\n\n";
  o << format_diagnostics(report.diagnostics, max_lines);
  return o.str();
}

} // namespace mtx
