#pragma once
#include <array>
#include <cstddef>
#include <string>

namespace mtx {

class GlyphMetricsTable {
public:
  GlyphMetricsTable();

  void set_fixed_width(int px);
  void set_width(unsigned char c, int px);
  int width(unsigned char c) const;
  int line_height() const noexcept;
  int baseline() const noexcept;
  void set_vertical_metrics(int line_height, int baseline);

  int measure_bytes(const std::string& s, std::size_t begin, std::size_t end) const;
  std::size_t byte_at_x(const std::string& s, std::size_t begin, std::size_t end, int pixel_x) const;

private:
  std::array<int, 256> widths_{};
  int line_h_{16};
  int baseline_{12};
};

} // namespace mtx
