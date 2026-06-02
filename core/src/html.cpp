#include "mtx/html.hpp"
#include "mtx/layout_engine.hpp"
#include "mtx/render_svg.hpp"
#include <cctype>
#include <string>

namespace mtx {

const char* html_theme_id(HtmlTheme theme) noexcept {
  switch (theme) {
    case HtmlTheme::Cyberpunk: return "cyberpunk";
    case HtmlTheme::Dystopia: return "dystopia";
    case HtmlTheme::Horror: return "horror";
    case HtmlTheme::Adventure: return "adventure";
    case HtmlTheme::Standard:
    default: return "standard";
  }
}

const char* html_theme_label(HtmlTheme theme) noexcept {
  switch (theme) {
    case HtmlTheme::Cyberpunk: return "Cyberpunk";
    case HtmlTheme::Dystopia: return "Dystopie";
    case HtmlTheme::Horror: return "Horror";
    case HtmlTheme::Adventure: return "Abenteuer";
    case HtmlTheme::Standard:
    default: return "Standard";
  }
}

HtmlTheme html_theme_from_id(const std::string& id) noexcept {
  std::string s;
  s.reserve(id.size());
  for (unsigned char c : id) s.push_back(static_cast<char>(std::tolower(c)));
  if (s == "cyberpunk" || s == "cyber") return HtmlTheme::Cyberpunk;
  if (s == "dystopia" || s == "dystopie") return HtmlTheme::Dystopia;
  if (s == "horror") return HtmlTheme::Horror;
  if (s == "adventure" || s == "abenteuer" || s == "spannend" || s == "thriller") return HtmlTheme::Adventure;
  return HtmlTheme::Standard;
}

std::string render_html(const std::string& markdown, HtmlTheme theme) {
  DisplayList list = markdown_to_display_list(markdown, theme, 760);
  return display_list_to_svg_html(list);
}

} // namespace mtx
