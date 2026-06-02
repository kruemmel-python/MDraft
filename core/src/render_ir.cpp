#include "mtx/render_ir.hpp"
#include <cstdint>
#include <utility>

namespace mtx {

DrawCommand make_rect(int x, int y, int w, int h, Rgba fill, Rgba stroke, int stroke_width) {
  DrawCommand c;
  c.kind = DrawKind::Rect;
  c.x = x;
  c.y = y;
  c.w = w;
  c.h = h;
  c.fill = fill;
  c.stroke = stroke;
  c.stroke_width = stroke_width;
  return c;
}

DrawCommand make_line(int x1, int y1, int x2, int y2, Rgba stroke, int stroke_width) {
  DrawCommand c;
  c.kind = DrawKind::Line;
  c.x = x1;
  c.y = y1;
  c.x2 = x2;
  c.y2 = y2;
  c.stroke = stroke;
  c.stroke_width = stroke_width;
  return c;
}

DrawCommand make_text(int x, int y, std::string text, Rgba fill, int font_size, TextWeight weight) {
  DrawCommand c;
  c.kind = DrawKind::Text;
  c.x = x;
  c.y = y;
  c.text = std::move(text);
  c.fill = fill;
  c.font_size = font_size;
  c.weight = weight;
  return c;
}


DrawCommand make_image(int x, int y, int w, int h, std::string path, std::string href, std::string alt, Rgba fill, Rgba stroke, int stroke_width) {
  DrawCommand c;
  c.kind = DrawKind::Image;
  c.x = x;
  c.y = y;
  c.w = w;
  c.h = h;
  c.path = std::move(path);
  c.href = std::move(href);
  c.text = std::move(alt);
  c.fill = fill;
  c.stroke = stroke;
  c.stroke_width = stroke_width;
  return c;
}

static void mix(std::uint64_t& h, std::uint64_t v) {
  h ^= v + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
}

std::uint64_t display_list_hash(const DisplayList& list) {
  std::uint64_t h = 1469598103934665603ull;
  mix(h, static_cast<std::uint64_t>(list.width));
  mix(h, static_cast<std::uint64_t>(list.height));
  mix(h, static_cast<std::uint64_t>(list.commands.size()));
  mix(h, static_cast<std::uint64_t>(list.math_box_count));
  mix(h, static_cast<std::uint64_t>(list.mermaid_node_count));
  mix(h, static_cast<std::uint64_t>(list.mermaid_edge_count));
  mix(h, static_cast<std::uint64_t>(list.image_count));
  for (const auto& c : list.commands) {
    mix(h, static_cast<std::uint64_t>(c.kind));
    mix(h, static_cast<std::uint64_t>(c.x));
    mix(h, static_cast<std::uint64_t>(c.y));
    mix(h, static_cast<std::uint64_t>(c.w));
    mix(h, static_cast<std::uint64_t>(c.h));
    mix(h, static_cast<std::uint64_t>(c.x2));
    mix(h, static_cast<std::uint64_t>(c.y2));
    mix(h, static_cast<std::uint64_t>(c.fill.r) << 24 | static_cast<std::uint64_t>(c.fill.g) << 16 |
           static_cast<std::uint64_t>(c.fill.b) << 8 | c.fill.a);
    mix(h, static_cast<std::uint64_t>(c.stroke.r) << 24 | static_cast<std::uint64_t>(c.stroke.g) << 16 |
           static_cast<std::uint64_t>(c.stroke.b) << 8 | c.stroke.a);
    for (unsigned char ch : c.text) mix(h, ch);
    for (unsigned char ch : c.path) mix(h, ch);
    for (unsigned char ch : c.href) mix(h, ch);
  }
  return h;
}

} // namespace mtx
