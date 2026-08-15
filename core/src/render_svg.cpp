#include "mtx/render_svg.hpp"
#include <algorithm>
#include <iomanip>
#include <sstream>

namespace mtx {
namespace {

std::string esc(const std::string& s) {
  std::string out;
  out.reserve(s.size() + 16);
  for (char c : s) {
    if (c == '&') out += "&amp;";
    else if (c == '<') out += "&lt;";
    else if (c == '>') out += "&gt;";
    else if (c == '"') out += "&quot;";
    else out += c;
  }
  return out;
}

std::string rgb(Rgba c) {
  std::ostringstream o;
  o << "rgb(" << static_cast<int>(c.r) << "," << static_cast<int>(c.g) << "," << static_cast<int>(c.b) << ")";
  return o.str();
}

const char* css_id(HtmlTheme theme) {
  return html_theme_id(theme);
}

std::string root_background(HtmlTheme theme) {
  switch (theme) {
    case HtmlTheme::GitHub:
      return "#ffffff";
    case HtmlTheme::Cyberpunk:
      return "radial-gradient(circle at 15% 5%,rgba(255,43,214,.28),transparent 28rem),radial-gradient(circle at 85% 10%,rgba(0,245,255,.20),transparent 24rem),linear-gradient(180deg,#060014,#090018 48%,#02030a)";
    case HtmlTheme::Dystopia:
      return "linear-gradient(180deg,#11100d,#1c1913 55%,#0b0b09)";
    case HtmlTheme::Adventure:
      return "radial-gradient(circle at 10% 0,rgba(31,111,91,.20),transparent 26rem),linear-gradient(180deg,#f7ead0,#e9d1a6)";
    case HtmlTheme::Standard:
      return "#ffffff";
    case HtmlTheme::Horror:
    default:
      return "radial-gradient(circle at 50% 0,rgba(138,3,3,.23),transparent 30rem),linear-gradient(180deg,#070404,#110707 60%,#020101)";
  }
}

const char* svg_font_family(TextFace face) noexcept {
  switch (face) {
    case TextFace::Sans:
      return "-apple-system, BlinkMacSystemFont, Segoe UI, Noto Sans, Helvetica, Arial, sans-serif";
    case TextFace::Monospace:
    default:
      return "ui-monospace, SFMono-Regular, SF Mono, Consolas, Liberation Mono, Menlo, monospace";
  }
}

} // namespace

std::string display_list_to_svg_html(const DisplayList& list) {
  std::ostringstream o;
  const int page_w = list.width + 80;
  const int page_h = list.height + 80;
  o << "<!doctype html>\n<html lang=\"de\">\n<head>\n<meta charset=\"utf-8\">\n"
    << "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
    << "<title>MDraft RenderIR Export</title>\n"
    << "<style>\n"
    << "html,body{margin:0;min-height:100%;background:" << rgb(list.background) << ";color:#eadfda;}\n"
    << "body{font-family:Consolas,'Cascadia Mono','Courier New',monospace;}\n"
    << ".render-root{box-sizing:border-box;min-height:100vh;padding:40px;background:" << root_background(list.theme) << ";}\n"
    << ".render-frame{display:block;margin:0 auto;max-width:calc(100vw - 32px);height:auto;}\n"
    << "</style>\n</head>\n<body class=\"theme-" << css_id(list.theme) << "\">\n"
    << "<main class=\"render-root\">\n"
    << "<svg class=\"render-frame\" xmlns=\"http://www.w3.org/2000/svg\" "
    << "width=\"" << list.width << "\" height=\"" << list.height << "\" "
    << "viewBox=\"0 0 " << list.width << " " << list.height << "\" role=\"img\" aria-label=\"MDraft RenderIR Export\">\n";

  (void)page_w;
  (void)page_h;
  for (const auto& c : list.commands) {
    switch (c.kind) {
      case DrawKind::Rect:
        o << "<rect x=\"" << c.x << "\" y=\"" << c.y << "\" width=\"" << c.w << "\" height=\"" << c.h
          << "\" fill=\"" << rgb(c.fill) << "\"";
        if (c.stroke_width > 0) {
          o << " stroke=\"" << rgb(c.stroke) << "\" stroke-width=\"" << c.stroke_width << "\"";
        }
        o << "/>\n";
        break;
      case DrawKind::Line:
        o << "<line x1=\"" << c.x << "\" y1=\"" << c.y << "\" x2=\"" << c.x2 << "\" y2=\"" << c.y2
          << "\" stroke=\"" << rgb(c.stroke) << "\" stroke-width=\"" << c.stroke_width << "\"/>\n";
        break;
      case DrawKind::Text:
        o << "<text x=\"" << c.x << "\" y=\"" << (c.y + c.font_size) << "\" fill=\"" << rgb(c.fill)
          << "\" font-family=\"" << svg_font_family(c.face) << "\" font-size=\"" << c.font_size << "\"";
        if (c.weight == TextWeight::Bold) o << " font-weight=\"700\"";
        o << ">" << esc(c.text) << "</text>\n";
        break;
      case DrawKind::Image: {
        const std::string href = c.href.empty() ? c.path : c.href;
        o << "<g>\n";
        o << "<rect x=\"" << c.x << "\" y=\"" << c.y << "\" width=\"" << c.w << "\" height=\"" << c.h
          << "\" fill=\"" << rgb(c.fill) << "\"";
        if (c.stroke_width > 0) o << " stroke=\"" << rgb(c.stroke) << "\" stroke-width=\"" << c.stroke_width << "\"";
        o << "/>\n";
        if (!href.empty()) {
          o << "<image x=\"" << (c.x + 1) << "\" y=\"" << (c.y + 1) << "\" width=\"" << std::max(0, c.w - 2)
            << "\" height=\"" << std::max(0, c.h - 2) << "\" href=\"" << esc(href)
            << "\" preserveAspectRatio=\"xMidYMid meet\"/>\n";
        }
        if (!c.text.empty()) {
          o << "<text x=\"" << (c.x + 10) << "\" y=\"" << (c.y + c.h - 12)
            << "\" fill=\"" << rgb(c.stroke) << "\" font-family=\"Consolas, Cascadia Mono, Courier New, monospace\" font-size=\"13\">"
            << esc(c.text) << "</text>\n";
        }
        o << "</g>\n";
        break;
      }
    }
  }

  o << "</svg>\n</main>\n</body>\n</html>\n";
  return o.str();
}

} // namespace mtx
