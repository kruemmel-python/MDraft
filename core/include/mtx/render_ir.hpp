#pragma once
#include "mtx/html.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace mtx {

struct Rgba {
  std::uint8_t r{0};
  std::uint8_t g{0};
  std::uint8_t b{0};
  std::uint8_t a{255};
};

enum class DrawKind {
  Rect,
  Line,
  Text,
  Image
};

enum class TextWeight {
  Regular,
  Bold
};

enum class TextFace {
  Monospace,
  Sans
};

struct DrawCommand {
  DrawKind kind{DrawKind::Rect};
  int x{0};
  int y{0};
  int w{0};
  int h{0};
  int x2{0};
  int y2{0};
  Rgba fill{};
  Rgba stroke{};
  int stroke_width{0};
  std::string text;
  std::string path;
  std::string href;
  int font_size{16};
  TextWeight weight{TextWeight::Regular};
  TextFace face{TextFace::Monospace};
};

struct DisplayList {
  int width{760};
  int height{0};
  HtmlTheme theme{HtmlTheme::Horror};
  Rgba background{};
  std::vector<DrawCommand> commands;
  int math_box_count{0};
  int mermaid_node_count{0};
  int mermaid_edge_count{0};
  int image_count{0};
};

DrawCommand make_rect(int x, int y, int w, int h, Rgba fill, Rgba stroke = Rgba{}, int stroke_width = 0);
DrawCommand make_line(int x1, int y1, int x2, int y2, Rgba stroke, int stroke_width = 1);
DrawCommand make_text(int x,
                      int y,
                      std::string text,
                      Rgba fill,
                      int font_size = 16,
                      TextWeight weight = TextWeight::Regular,
                      TextFace face = TextFace::Monospace);
DrawCommand make_image(int x, int y, int w, int h, std::string path, std::string href, std::string alt, Rgba fill, Rgba stroke, int stroke_width = 1);

std::uint64_t display_list_hash(const DisplayList& list);

} // namespace mtx
