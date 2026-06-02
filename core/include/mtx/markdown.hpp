#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mtx {

enum class BlockKind : std::uint8_t {
  Paragraph, Heading, Quote, ListItem, TaskItem, CodeFence, MathBlock, TableRow, Blank
};

struct Block {
  BlockKind kind{BlockKind::Paragraph};
  std::size_t begin{0};
  std::size_t end{0};
  std::uint8_t level{0};
  bool checked{false};
  std::string info;
};

enum class TextStyle : std::uint16_t {
  Normal    = 0,
  Heading1  = 1 << 0,
  Heading2  = 1 << 1,
  Heading3  = 1 << 2,
  Bold      = 1 << 3,
  Italic    = 1 << 4,
  Code      = 1 << 5,
  Quote     = 1 << 6,
  List      = 1 << 7,
  Math      = 1 << 8,
  Selection = 1 << 9
};

inline TextStyle operator|(TextStyle a, TextStyle b) {
  return static_cast<TextStyle>(static_cast<std::uint16_t>(a) | static_cast<std::uint16_t>(b));
}
inline TextStyle& operator|=(TextStyle& a, TextStyle b) { a = a | b; return a; }
inline bool has_style(TextStyle a, TextStyle b) {
  return (static_cast<std::uint16_t>(a) & static_cast<std::uint16_t>(b)) != 0;
}

struct StyledSpan {
  std::size_t begin{0};
  std::size_t end{0};
  TextStyle style{TextStyle::Normal};
};

std::vector<Block> parse_blocks(const std::string& text);
std::vector<StyledSpan> parse_inline_styles(const std::string& text);

} // namespace mtx
