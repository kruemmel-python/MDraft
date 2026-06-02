#pragma once
#include "mtx/render_ir.hpp"
#include <string>

namespace mtx {

DisplayList markdown_to_display_list(const std::string& markdown,
                                     HtmlTheme theme = HtmlTheme::Horror,
                                     int width = 760);

DisplayList markdown_to_display_list_with_base(const std::string& markdown,
                                               HtmlTheme theme,
                                               int width,
                                               const std::string& base_path);

} // namespace mtx
