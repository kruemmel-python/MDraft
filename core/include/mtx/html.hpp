#pragma once
#include <string>

namespace mtx {

enum class HtmlTheme {
  Standard,
  Cyberpunk,
  Dystopia,
  Horror,
  Adventure
};

const char* html_theme_id(HtmlTheme theme) noexcept;
const char* html_theme_label(HtmlTheme theme) noexcept;
HtmlTheme html_theme_from_id(const std::string& id) noexcept;

std::string render_html(const std::string& markdown, HtmlTheme theme = HtmlTheme::Horror);

} // namespace mtx
