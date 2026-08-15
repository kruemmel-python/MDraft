#include "mtx/glyph_metrics_table.hpp"
#include <algorithm>

namespace mtx {
namespace {
bool is_utf8_continuation(unsigned char c) noexcept {
  return (c & 0xC0) == 0x80;
}

std::size_t next_codepoint_start(const std::string& s, std::size_t i, std::size_t end) noexcept {
  i = std::min(i + 1, end);
  while (i < end && is_utf8_continuation(static_cast<unsigned char>(s[i]))) ++i;
  return i;
}
}

GlyphMetricsTable::GlyphMetricsTable() { set_fixed_width(8); }

void GlyphMetricsTable::set_fixed_width(int px) {
  px = std::max(1, px);
  widths_.fill(px);
  widths_[static_cast<unsigned char>('\t')] = px * 4;
  widths_[static_cast<unsigned char>('\n')] = 0;
}

void GlyphMetricsTable::set_width(unsigned char c, int px) { widths_[c] = std::max(0, px); }
int GlyphMetricsTable::width(unsigned char c) const { return widths_[c]; }
int GlyphMetricsTable::line_height() const noexcept { return line_h_; }
int GlyphMetricsTable::baseline() const noexcept { return baseline_; }

void GlyphMetricsTable::set_vertical_metrics(int line_height, int baseline) {
  line_h_ = std::max(1, line_height);
  baseline_ = std::max(1, std::min(baseline, line_h_));
}

int GlyphMetricsTable::measure_bytes(const std::string& s, std::size_t begin, std::size_t end) const {
  if (begin > end) std::swap(begin, end);
  end = std::min(end, s.size());
  int x = 0;
  for (std::size_t i = begin; i < end; ++i) {
    if (!is_utf8_continuation(static_cast<unsigned char>(s[i]))) x += width(static_cast<unsigned char>(s[i]));
  }
  return x;
}

std::size_t GlyphMetricsTable::byte_at_x(const std::string& s, std::size_t begin, std::size_t end, int pixel_x) const {
  if (begin > end) std::swap(begin, end);
  end = std::min(end, s.size());
  int x = 0;
  for (std::size_t i = begin; i < end; ++i) {
    const unsigned char c = static_cast<unsigned char>(s[i]);
    if (is_utf8_continuation(c)) continue;
    const int w = width(c);
    if (pixel_x < x + w / 2) return i;
    x += w;
    const std::size_t next = next_codepoint_start(s, i, end);
    if (pixel_x < x && next > i + 1) return next;
  }
  return end;
}

} // namespace mtx
