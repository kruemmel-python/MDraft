#include "mtx/layout_engine.hpp"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace mtx {
namespace {

struct ThemePalette {
  Rgba bg;
  Rgba panel;
  Rgba soft;
  Rgba text;
  Rgba muted;
  Rgba accent;
  Rgba border;
  Rgba math_bg;
  Rgba math_text;
};

ThemePalette palette(HtmlTheme theme) {
  switch (theme) {
    case HtmlTheme::Cyberpunk:
      return {{6,0,20,255},{10,0,28,255},{19,0,43,255},{232,251,255,255},{123,223,242,255},{255,43,214,255},{0,245,255,255},{19,0,43,255},{0,245,255,255}};
    case HtmlTheme::Dystopia:
      return {{21,19,15,255},{33,31,25,255},{26,24,20,255},{216,210,191,255},{155,146,126,255},{211,155,70,255},{74,67,55,255},{17,16,13,255},{227,196,130,255}};
    case HtmlTheme::Adventure:
      return {{247,234,208,255},{255,247,223,255},{255,240,200,255},{31,43,33,255},{91,104,78,255},{31,111,91,255},{184,138,59,255},{240,223,184,255},{92,59,18,255}};
    case HtmlTheme::Standard:
      return {{255,255,255,255},{255,255,255,255},{246,248,250,255},{36,41,47,255},{87,96,106,255},{9,105,218,255},{208,215,222,255},{246,248,250,255},{130,80,223,255}};
    case HtmlTheme::Horror:
    default:
      return {{7,4,4,255},{14,7,7,255},{18,9,9,255},{234,223,218,255},{162,143,139,255},{208,24,24,255},{76,23,23,255},{246,248,250,255},{130,80,223,255}};
  }
}

std::string trim(std::string s) {
  auto not_space = [](unsigned char c) { return !std::isspace(c); };
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), not_space));
  s.erase(std::find_if(s.rbegin(), s.rend(), not_space).base(), s.end());
  return s;
}

bool starts_with(const std::string& s, const std::string& p) {
  return s.size() >= p.size() && std::equal(p.begin(), p.end(), s.begin());
}

std::string lower_ascii(std::string s) {
  for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return s;
}

std::vector<std::string> split_lines(const std::string& text) {
  std::vector<std::string> lines;
  std::size_t p = 0;
  while (p <= text.size()) {
    std::size_t e = text.find('\n', p);
    if (e == std::string::npos) e = text.size();
    std::string line = text.substr(p, e - p);
    if (!line.empty() && line.back() == '\r') line.pop_back();
    lines.push_back(line);
    if (e == text.size()) break;
    p = e + 1;
  }
  return lines;
}

bool is_hr(const std::string& line) {
  const std::string t = trim(line);
  if (t.size() < 3) return false;
  const char c = t[0];
  if (c != '-' && c != '*' && c != '_') return false;
  for (char x : t) {
    if (x != c && !std::isspace(static_cast<unsigned char>(x))) return false;
  }
  return true;
}

std::string strip_inline(std::string s) {
  std::string out;
  out.reserve(s.size());
  for (std::size_t i = 0; i < s.size();) {
    if (i + 1 < s.size() && s[i] == '*' && s[i + 1] == '*') { i += 2; continue; }
    if (s[i] == '*' || s[i] == '`') { ++i; continue; }
    if (s[i] == '$') {
      out += "∑ ";
      ++i;
      while (i < s.size() && s[i] != '$') out.push_back(s[i++]);
      if (i < s.size() && s[i] == '$') ++i;
      continue;
    }
    out.push_back(s[i++]);
  }
  return out;
}

std::vector<std::string> wrap_text(const std::string& text, int max_chars) {
  std::vector<std::string> out;
  if (max_chars < 8) max_chars = 8;
  std::istringstream is(text);
  std::string word;
  std::string line;
  while (is >> word) {
    if (!line.empty() && static_cast<int>(line.size() + 1 + word.size()) > max_chars) {
      out.push_back(line);
      line.clear();
    }
    if (!line.empty()) line += " ";
    line += word;
  }
  if (!line.empty()) out.push_back(line);
  if (out.empty()) out.push_back("");
  return out;
}

bool is_table_row(const std::string& line) {
  const std::string t = trim(line);
  return t.size() >= 3 && t.find('|') != std::string::npos;
}

bool is_table_separator(const std::string& line) {
  const std::string t = trim(line);
  if (t.empty() || t.find('|') == std::string::npos || t.find('-') == std::string::npos) return false;
  for (char c : t) {
    if (c != '|' && c != '-' && c != ':' && !std::isspace(static_cast<unsigned char>(c))) return false;
  }
  return true;
}

std::vector<std::string> table_cells(const std::string& row) {
  std::vector<std::string> cells;
  std::string s = trim(row);
  if (!s.empty() && s.front() == '|') s.erase(s.begin());
  if (!s.empty() && s.back() == '|') s.pop_back();
  std::string cur;
  for (char c : s) {
    if (c == '|') {
      cells.push_back(strip_inline(trim(cur)));
      cur.clear();
    } else {
      cur.push_back(c);
    }
  }
  cells.push_back(strip_inline(trim(cur)));
  return cells;
}

struct ImageRef {
  bool valid{false};
  std::string alt;
  std::string href;
};

ImageRef parse_image_line(const std::string& line) {
  const std::string t = trim(line);
  const std::size_t bang = t.find("![");
  if (bang == std::string::npos) return {};
  const std::size_t alt_end = t.find("](", bang + 2);
  if (alt_end == std::string::npos) return {};
  const std::size_t href_begin = alt_end + 2;
  const std::size_t href_end = t.find(')', href_begin);
  if (href_end == std::string::npos || href_end <= href_begin) return {};
  ImageRef ref;
  ref.valid = true;
  ref.alt = t.substr(bang + 2, alt_end - (bang + 2));
  ref.href = t.substr(href_begin, href_end - href_begin);
  const std::size_t title_quote = ref.href.find('"');
  if (title_quote != std::string::npos) ref.href = trim(ref.href.substr(0, title_quote));
  return ref;
}

bool is_url_like(const std::string& href) {
  const std::string low = lower_ascii(href);
  return starts_with(low, "http://") || starts_with(low, "https://") || starts_with(low, "data:");
}

std::string resolve_asset_path(const std::string& base_path, const std::string& href) {
  if (href.empty() || is_url_like(href)) return href;
  std::filesystem::path p = std::filesystem::u8path(href);
  if (p.is_absolute() || base_path.empty()) return p.u8string();
  std::filesystem::path base = std::filesystem::u8path(base_path);
  if (std::filesystem::is_regular_file(base)) base = base.parent_path();
  return (base / p).lexically_normal().u8string();
}

std::string replace_all(std::string s, const std::string& from, const std::string& to) {
  if (from.empty()) return s;
  std::size_t p = 0;
  while ((p = s.find(from, p)) != std::string::npos) {
    s.replace(p, from.size(), to);
    p += to.size();
  }
  return s;
}

std::string render_math_text(std::string s) {
  s = trim(s);
  const std::vector<std::pair<std::string, std::string>> repl = {
    {"\\alpha","α"},{"\\beta","β"},{"\\gamma","γ"},{"\\delta","δ"},{"\\epsilon","ε"},
    {"\\lambda","λ"},{"\\mu","μ"},{"\\pi","π"},{"\\sigma","σ"},{"\\phi","φ"},{"\\omega","ω"},
    {"\\Delta","Δ"},{"\\Sigma","Σ"},{"\\Omega","Ω"},{"\\partial","∂"},{"\\nabla","∇"},
    {"\\hbar","ℏ"},{"\\times","×"},{"\\cdot","·"},{"\\to","→"},{"\\rightarrow","→"},
    {"\\left",""},{"\\right",""},{"\\mathbf",""},{"\\mathrm",""},{"\\log","log"},{"\\sum","∑"},{"\\int","∫"}
  };
  for (const auto& r : repl) s = replace_all(s, r.first, r.second);

  // Minimal structural projection for \frac{a}{b}; not full TeX, but stable RenderIR.
  for (;;) {
    const std::string tag = "\\frac{";
    const std::size_t p = s.find(tag);
    if (p == std::string::npos) break;
    const std::size_t a0 = p + tag.size();
    const std::size_t a1 = s.find('}', a0);
    if (a1 == std::string::npos || a1 + 1 >= s.size() || s[a1 + 1] != '{') break;
    const std::size_t b0 = a1 + 2;
    const std::size_t b1 = s.find('}', b0);
    if (b1 == std::string::npos) break;
    const std::string a = s.substr(a0, a1 - a0);
    const std::string b = s.substr(b0, b1 - b0);
    s.replace(p, b1 - p + 1, "(" + a + ")/(" + b + ")");
  }

  s = replace_all(s, "^{2}", "²");
  s = replace_all(s, "^2", "²");
  s = replace_all(s, "_i", "ᵢ");
  s = replace_all(s, "_n", "ₙ");
  s = replace_all(s, "{", "");
  s = replace_all(s, "}", "");
  return s;
}

std::string node_id(std::string s) {
  s = trim(s);
  const std::size_t bracket = s.find_first_of("[({");
  if (bracket != std::string::npos) s = trim(s.substr(0, bracket));
  while (!s.empty() && (s.back() == ';' || s.back() == ',')) s.pop_back();
  return trim(s);
}

std::string node_label(const std::string& raw) {
  const std::string s = trim(raw);
  const std::size_t open = s.find_first_of("[({");
  if (open == std::string::npos) return node_id(s);
  const char close = s[open] == '[' ? ']' : (s[open] == '(' ? ')' : '}');
  const std::size_t end = s.find(close, open + 1);
  if (end == std::string::npos || end <= open + 1) return node_id(s);
  std::string label = s.substr(open + 1, end - open - 1);
  if (label.size() >= 2 && label.front() == '"' && label.back() == '"') label = label.substr(1, label.size() - 2);
  return trim(label);
}

void add_text(DisplayList& dl, int x, int y, const std::string& text, Rgba color, int font, TextWeight w = TextWeight::Regular) {
  dl.commands.push_back(make_text(x, y, text, color, font, w));
}

void add_paragraph(DisplayList& dl, int& y, const ThemePalette& p, const std::string& text, int x, int width) {
  const int max_chars = std::max(12, width / 10);
  for (const auto& line : wrap_text(strip_inline(text), max_chars)) {
    add_text(dl, x, y, line, p.text, 16);
    y += 24;
  }
  y += 8;
}

void add_math_block(DisplayList& dl, int& y, const ThemePalette& p, const std::vector<std::string>& lines, int x, int width) {
  const int h = std::max(58, 28 + static_cast<int>(lines.size()) * 24);
  dl.commands.push_back(make_rect(x, y, width, h, p.math_bg, p.border, 1));
  int yy = y + 18;
  for (const auto& raw : lines) {
    const std::string t = render_math_text(raw);
    const int text_px = static_cast<int>(t.size()) * 8;
    add_text(dl, x + std::max(18, (width - text_px) / 2), yy, "ƒ " + t, p.math_text, 16);
    yy += 24;
  }
  ++dl.math_box_count;
  y += h + 18;
}

void add_image_block(DisplayList& dl, int& y, const ThemePalette& p, const ImageRef& ref, const std::string& base_path, int x, int width) {
  const int h = std::max(120, std::min(360, (width * 9) / 16));
  const std::string resolved = resolve_asset_path(base_path, ref.href);
  dl.commands.push_back(make_rect(x, y, width, h + 34, p.soft, p.border, 1));
  dl.commands.push_back(make_image(x + 12, y + 12, width - 24, h, resolved, ref.href, ref.alt, p.panel, p.border, 1));
  const std::string caption = ref.alt.empty() ? ref.href : ref.alt + " · " + ref.href;
  add_text(dl, x + 12, y + h + 20, caption.substr(0, 96), p.muted, 13);
  ++dl.image_count;
  y += h + 52;
}

void add_table(DisplayList& dl, int& y, const ThemePalette& p, const std::vector<std::string>& rows, int x, int width) {
  if (rows.empty()) return;
  std::vector<std::vector<std::string>> cells;
  std::size_t cols = 0;
  for (const auto& r : rows) {
    cells.push_back(table_cells(r));
    cols = std::max(cols, cells.back().size());
  }
  if (cols == 0) return;
  const int row_h = 32;
  const int table_h = static_cast<int>(cells.size()) * row_h;
  const int col_w = std::max(48, width / static_cast<int>(cols));
  dl.commands.push_back(make_rect(x, y, col_w * static_cast<int>(cols), table_h, p.panel, p.border, 1));
  for (std::size_t r = 0; r < cells.size(); ++r) {
    const int yy = y + static_cast<int>(r) * row_h;
    const Rgba bg = (r == 0) ? p.soft : ((r % 2 == 0) ? p.panel : p.bg);
    dl.commands.push_back(make_rect(x, yy, col_w * static_cast<int>(cols), row_h, bg, p.border, 1));
    for (std::size_t c = 0; c < cols; ++c) {
      const int xx = x + static_cast<int>(c) * col_w;
      dl.commands.push_back(make_line(xx, yy, xx, yy + row_h, p.border, 1));
      const std::string value = (c < cells[r].size()) ? cells[r][c] : std::string{};
      const int max_chars = std::max(4, (col_w - 16) / 10);
      std::string shown = value;
      if (static_cast<int>(shown.size()) > max_chars) shown = shown.substr(0, static_cast<std::size_t>(max_chars - 1)) + "…";
      add_text(dl, xx + 8, yy + 8, shown, p.text, 14, r == 0 ? TextWeight::Bold : TextWeight::Regular);
    }
    dl.commands.push_back(make_line(x + col_w * static_cast<int>(cols), yy, x + col_w * static_cast<int>(cols), yy + row_h, p.border, 1));
  }
  for (std::size_t r = 0; r <= cells.size(); ++r) {
    const int yy = y + static_cast<int>(r) * row_h;
    dl.commands.push_back(make_line(x, yy, x + col_w * static_cast<int>(cols), yy, p.border, 1));
  }
  y += table_h + 18;
}

struct Node { std::string id; std::string label; int x{0}; int y{0}; };
struct Edge { int a{0}; int b{0}; std::string label; };

void add_mermaid(DisplayList& dl, int& y, const ThemePalette& p, const std::vector<std::string>& source, int x, int width) {
  std::vector<Node> nodes;
  std::vector<Edge> edges;
  bool lr = true;

  auto add_node = [&](const std::string& raw) -> int {
    const std::string id = node_id(raw);
    if (id.empty()) return -1;
    for (std::size_t i = 0; i < nodes.size(); ++i) if (nodes[i].id == id) return static_cast<int>(i);
    nodes.push_back({id, node_label(raw), 0, 0});
    return static_cast<int>(nodes.size() - 1);
  };

  for (std::string line : source) {
    line = trim(line);
    if (line.empty() || starts_with(line, "%%")) continue;
    const std::string low = lower_ascii(line);
    if (starts_with(low, "graph ") || starts_with(low, "flowchart ")) {
      lr = low.find(" lr") != std::string::npos;
      continue;
    }
    std::size_t arrow = line.find("-->");
    if (arrow == std::string::npos) arrow = line.find("---");
    if (arrow == std::string::npos) continue;
    std::string left = trim(line.substr(0, arrow));
    std::string right = trim(line.substr(arrow + 3));
    std::string label;
    if (!right.empty() && right[0] == '|') {
      const std::size_t e = right.find('|', 1);
      if (e != std::string::npos) {
        label = trim(right.substr(1, e - 1));
        right = trim(right.substr(e + 1));
      }
    }
    const int a = add_node(left);
    const int b = add_node(right);
    if (a >= 0 && b >= 0) edges.push_back({a, b, label});
  }

  add_text(dl, x, y, "Mermaid-Diagramm", p.accent, 16, TextWeight::Bold);
  y += 28;
  const int fig_h = std::max(150, std::min(260, 110 + static_cast<int>(nodes.size()) * 22));
  dl.commands.push_back(make_rect(x, y, width, fig_h, {11,3,3,255}, p.border, 1));
  const int node_w = 112;
  const int node_h = 30;

  for (std::size_t i = 0; i < nodes.size(); ++i) {
    if (lr) {
      const int usable = std::max(1, width - 80 - node_w);
      nodes[i].x = x + 40 + static_cast<int>((usable * i) / std::max<std::size_t>(1, nodes.size() - 1));
      nodes[i].y = y + 36 + (static_cast<int>(i) % 2) * 64;
    } else {
      nodes[i].x = x + 30 + (static_cast<int>(i) % 3) * 150;
      nodes[i].y = y + 30 + static_cast<int>(i) * 48;
    }
    if (nodes[i].x + node_w > x + width - 20) nodes[i].x = x + width - node_w - 20;
    if (nodes[i].y + node_h > y + fig_h - 20) nodes[i].y = y + fig_h - node_h - 20;
  }

  for (const auto& e : edges) {
    if (e.a < 0 || e.b < 0 || e.a >= static_cast<int>(nodes.size()) || e.b >= static_cast<int>(nodes.size())) continue;
    const Node& a = nodes[static_cast<std::size_t>(e.a)];
    const Node& b = nodes[static_cast<std::size_t>(e.b)];
    const int ax = a.x + node_w / 2;
    const int ay = a.y + node_h / 2;
    const int bx = b.x + node_w / 2;
    const int by = b.y + node_h / 2;
    dl.commands.push_back(make_line(ax, ay, bx, by, p.muted, 1));
    dl.commands.push_back(make_rect(bx - 3, by - 3, 6, 6, p.muted));
    if (!e.label.empty()) add_text(dl, (ax + bx) / 2 - 40, (ay + by) / 2 - 22, e.label, p.text, 12);
  }

  for (const auto& n : nodes) {
    dl.commands.push_back(make_rect(n.x, n.y, node_w, node_h, p.panel, p.accent, 1));
    add_text(dl, n.x + 8, n.y + 7, n.label.substr(0, 12), p.text, 12);
  }

  dl.mermaid_node_count += static_cast<int>(nodes.size());
  dl.mermaid_edge_count += static_cast<int>(edges.size());
  y += fig_h + 18;
}

std::vector<std::string> wrap_code_line(const std::string& text, int max_chars) {
  std::vector<std::string> out;
  if (max_chars < 12) max_chars = 12;
  if (static_cast<int>(text.size()) <= max_chars) {
    out.push_back(text);
    return out;
  }
  std::size_t start = 0;
  bool first = true;
  while (start < text.size()) {
    const int current_max = first ? max_chars : (max_chars - 4);
    std::size_t len = text.size() - start;
    if (static_cast<int>(len) <= current_max) {
      std::string segment = text.substr(start);
      if (!first) segment = "    " + segment;
      out.push_back(segment);
      break;
    }
    std::size_t split_pos = start + static_cast<std::size_t>(current_max);
    std::size_t space = text.find_last_of(" \t", split_pos);
    if (space != std::string::npos && space > start) {
      std::string segment = text.substr(start, space - start);
      if (!first) segment = "    " + segment;
      out.push_back(segment);
      start = space + 1;
    } else {
      std::string segment = text.substr(start, static_cast<std::size_t>(current_max));
      if (!first) segment = "    " + segment;
      out.push_back(segment);
      start += static_cast<std::size_t>(current_max);
    }
    first = false;
  }
  if (out.empty()) out.push_back("");
  return out;
}

} // namespace

DisplayList markdown_to_display_list_with_base(const std::string& markdown, HtmlTheme theme, int width, const std::string& base_path) {
  if (width < 320) width = 320;
  const ThemePalette p = palette(theme);
  DisplayList dl;
  dl.width = width;
  dl.theme = theme;
  dl.background = p.bg;

  int y = 0;
  dl.commands.push_back(make_rect(0, 0, width, 32, p.bg));
  y += 24;
  dl.commands.push_back(make_rect(0, y, width, 1, p.border));
  y += 18;

  const int x = 22;
  const int content_w = width - 44;
  const std::vector<std::string> lines = split_lines(markdown);
  bool in_code = false;
  bool in_mermaid = false;
  bool in_math = false;
  std::vector<std::string> fence;
  std::string paragraph;

  auto flush_paragraph = [&]() {
    if (!paragraph.empty()) {
      add_paragraph(dl, y, p, paragraph, x, content_w);
      paragraph.clear();
    }
  };

  for (std::size_t i = 0; i < lines.size(); ++i) {
    const std::string& raw = lines[i];
    const std::string t = trim(raw);

    if (starts_with(t, "```")) {
      if (!in_code) {
        flush_paragraph();
        in_code = true;
        in_mermaid = lower_ascii(t).find("mermaid") != std::string::npos;
        fence.clear();
      } else {
        if (in_mermaid) {
          add_mermaid(dl, y, p, fence, x, content_w);
        } else {
          std::vector<std::string> wrapped_fence;
          for (const auto& l : fence) {
            auto wrapped = wrap_code_line(l, 80);
            wrapped_fence.insert(wrapped_fence.end(), wrapped.begin(), wrapped.end());
          }
          const int h = std::max(34, 12 + static_cast<int>(wrapped_fence.size()) * 22);
          dl.commands.push_back(make_rect(x, y, content_w, h, {11,3,3,255}, p.border, 1));
          int yy = y + 8;
          for (const auto& l : wrapped_fence) {
            add_text(dl, x + 10, yy, l, {255,214,207,255}, 14);
            yy += 22;
          }
          y += h + 16;
        }
        in_code = false;
        in_mermaid = false;
        fence.clear();
      }
      continue;
    }
    if (in_code) { fence.push_back(raw); continue; }

    if (t == "$$") {
      if (!in_math) { flush_paragraph(); in_math = true; fence.clear(); }
      else { add_math_block(dl, y, p, fence, x, content_w); in_math = false; fence.clear(); }
      continue;
    }
    if (in_math) { fence.push_back(raw); continue; }

    if (t.empty()) {
      flush_paragraph();
      y += 6;
      continue;
    }

    const ImageRef img = parse_image_line(t);
    if (img.valid) {
      flush_paragraph();
      add_image_block(dl, y, p, img, base_path, x, content_w);
      continue;
    }

    if (i + 1 < lines.size() && is_table_row(t) && is_table_separator(lines[i + 1])) {
      flush_paragraph();
      std::vector<std::string> rows;
      rows.push_back(t);
      i += 2; // skip separator
      while (i < lines.size() && is_table_row(lines[i]) && !trim(lines[i]).empty()) {
        rows.push_back(lines[i]);
        ++i;
      }
      if (i < lines.size()) --i;
      add_table(dl, y, p, rows, x, content_w);
      continue;
    }

    if (is_hr(t)) {
      flush_paragraph();
      dl.commands.push_back(make_line(x, y + 8, x + content_w, y + 8, p.border, 1));
      y += 24;
      continue;
    }
    if (starts_with(t, "# ")) {
      flush_paragraph();
      add_text(dl, x, y, strip_inline(t.substr(2)), {255,241,238,255}, 24, TextWeight::Bold);
      y += 34;
      dl.commands.push_back(make_line(x, y, x + content_w, y, p.border, 1));
      y += 16;
      continue;
    }
    if (starts_with(t, "## ")) {
      flush_paragraph();
      add_text(dl, x, y, strip_inline(t.substr(3)), {255,232,226,255}, 20, TextWeight::Bold);
      y += 30;
      dl.commands.push_back(make_line(x, y, x + content_w, y, p.border, 1));
      y += 14;
      continue;
    }
    if (starts_with(t, "### ")) {
      flush_paragraph();
      add_text(dl, x, y, strip_inline(t.substr(4)), {255,210,210,255}, 18, TextWeight::Bold);
      y += 30;
      continue;
    }
    if (starts_with(t, "- [x] ") || starts_with(t, "- [X] ")) {
      flush_paragraph(); add_paragraph(dl, y, p, "☑ " + t.substr(6), x + 18, content_w - 18); continue;
    }
    if (starts_with(t, "- [ ] ")) {
      flush_paragraph(); add_paragraph(dl, y, p, "☐ " + t.substr(6), x + 18, content_w - 18); continue;
    }
    if (starts_with(t, "- ") || starts_with(t, "* ")) {
      flush_paragraph(); add_paragraph(dl, y, p, "• " + t.substr(2), x + 18, content_w - 18); continue;
    }
    if (starts_with(t, ">")) {
      flush_paragraph();
      const std::string quote_text = strip_inline(trim(t.substr(1)));
      const int max_chars = std::max(12, (content_w - 24) / 10);
      const auto quote_lines = wrap_text(quote_text, max_chars);
      const int h = static_cast<int>(quote_lines.size()) * 24 + 6;
      dl.commands.push_back(make_rect(x, y - 2, 3, h - 4, p.accent));
      dl.commands.push_back(make_rect(x + 8, y - 4, content_w - 8, h, p.soft));
      int yy = y;
      for (const auto& line : quote_lines) {
        add_text(dl, x + 16, yy, line, {200,170,165,255}, 16);
        yy += 24;
      }
      y += h + 6;
      continue;
    }

    if (!paragraph.empty()) paragraph += " ";
    paragraph += t;
  }

  flush_paragraph();
  if (in_math) add_math_block(dl, y, p, fence, x, content_w);
  if (in_code && in_mermaid) add_mermaid(dl, y, p, fence, x, content_w);
  y += 24;
  dl.height = std::max(y, 200);
  dl.commands.insert(dl.commands.begin(), make_rect(0, 0, width, dl.height, p.panel, p.border, 1));
  return dl;
}


DisplayList markdown_to_display_list(const std::string& markdown, HtmlTheme theme, int width) {
  return markdown_to_display_list_with_base(markdown, theme, width, std::string{});
}

} // namespace mtx
