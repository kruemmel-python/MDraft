#include "mtx/markdown.hpp"
#include <algorithm>

namespace mtx {

static bool starts_with(const std::string& s, std::size_t p, const char* lit) {
  for (std::size_t i = 0; lit[i]; ++i)
    if (p + i >= s.size() || s[p + i] != lit[i]) return false;
  return true;
}

std::vector<Block> parse_blocks(const std::string& text) {
  std::vector<Block> blocks;
  bool in_code = false;
  bool in_math = false;

  std::size_t line_begin = 0;
  while (line_begin <= text.size()) {
    std::size_t line_end = text.find('\n', line_begin);
    if (line_end == std::string::npos) line_end = text.size();
    const std::size_t len = line_end - line_begin;

    Block b;
    b.begin = line_begin;
    b.end = line_end;

    if (starts_with(text, line_begin, "```")) {
      b.kind = BlockKind::CodeFence;
      in_code = !in_code;
    } else if (starts_with(text, line_begin, "$$")) {
      b.kind = BlockKind::MathBlock;
      in_math = !in_math;
    } else if (in_code) {
      b.kind = BlockKind::CodeFence;
    } else if (in_math) {
      b.kind = BlockKind::MathBlock;
    } else if (len == 0) {
      b.kind = BlockKind::Blank;
    } else if (text[line_begin] == '#') {
      std::size_t n = 0;
      while (line_begin + n < line_end && text[line_begin + n] == '#') ++n;
      if (n > 0 && n <= 6 && line_begin + n < line_end && text[line_begin + n] == ' ') {
        b.kind = BlockKind::Heading;
        b.level = static_cast<std::uint8_t>(n);
      } else {
        b.kind = BlockKind::Paragraph;
      }
    } else if (text[line_begin] == '>') {
      b.kind = BlockKind::Quote;
    } else if (starts_with(text, line_begin, "- [x] ") || starts_with(text, line_begin, "- [X] ")) {
      b.kind = BlockKind::TaskItem;
      b.checked = true;
    } else if (starts_with(text, line_begin, "- [ ] ")) {
      b.kind = BlockKind::TaskItem;
      b.checked = false;
    } else if (starts_with(text, line_begin, "- ") || starts_with(text, line_begin, "* ")) {
      b.kind = BlockKind::ListItem;
    } else if (line_begin < line_end && text[line_begin] == '|') {
      b.kind = BlockKind::TableRow;
    } else {
      b.kind = BlockKind::Paragraph;
    }

    blocks.push_back(b);
    if (line_end == text.size()) break;
    line_begin = line_end + 1;
  }

  return blocks;
}

static void add_pair_spans(const std::string& text, std::vector<StyledSpan>& out,
                           const std::string& delim, TextStyle style) {
  std::size_t p = 0;
  while (p < text.size()) {
    std::size_t a = text.find(delim, p);
    if (a == std::string::npos) break;
    std::size_t b = text.find(delim, a + delim.size());
    if (b == std::string::npos) break;
    out.push_back({a, b + delim.size(), style});
    p = b + delim.size();
  }
}

std::vector<StyledSpan> parse_inline_styles(const std::string& text) {
  std::vector<StyledSpan> spans;
  for (const auto& b : parse_blocks(text)) {
    TextStyle s = TextStyle::Normal;
    switch (b.kind) {
      case BlockKind::Heading:
        s = b.level == 1 ? TextStyle::Heading1 : (b.level == 2 ? TextStyle::Heading2 : TextStyle::Heading3);
        break;
      case BlockKind::Quote: s = TextStyle::Quote; break;
      case BlockKind::ListItem:
      case BlockKind::TaskItem: s = TextStyle::List; break;
      case BlockKind::CodeFence: s = TextStyle::Code; break;
      case BlockKind::MathBlock: s = TextStyle::Math; break;
      default: break;
    }
    if (s != TextStyle::Normal) spans.push_back({b.begin, b.end, s});
  }

  add_pair_spans(text, spans, "**", TextStyle::Bold);
  add_pair_spans(text, spans, "`", TextStyle::Code);
  add_pair_spans(text, spans, "$", TextStyle::Math);

  // Conservative italic pass: single '*' not part of '**'.
  std::size_t p = 0;
  while (p < text.size()) {
    std::size_t a = text.find('*', p);
    if (a == std::string::npos) break;
    if ((a + 1 < text.size() && text[a+1] == '*') || (a > 0 && text[a-1] == '*')) { p = a + 1; continue; }
    std::size_t b = text.find('*', a + 1);
    if (b == std::string::npos) break;
    if (b > 0 && text[b-1] == '*') { p = b + 1; continue; }
    spans.push_back({a, b + 1, TextStyle::Italic});
    p = b + 1;
  }

  std::sort(spans.begin(), spans.end(), [](const StyledSpan& a, const StyledSpan& b) {
    if (a.begin != b.begin) return a.begin < b.begin;
    return a.end < b.end;
  });
  return spans;
}

} // namespace mtx
