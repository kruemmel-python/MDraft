#include "mtx/html.hpp"
#include "mtx/markdown.hpp"
#include <cstddef>
#include <cstring>
#include <string>

#if defined(_WIN32)
#define MDRAFT_API extern "C" __declspec(dllexport)
#else
#define MDRAFT_API extern "C" __attribute__((visibility("default")))
#endif

// Stable C ABI for native hosts. The host owns all provided memory.
MDRAFT_API std::size_t mdraft_render_html(const char* markdown,
                                          const char* theme_id,
                                          char* output,
                                          std::size_t capacity) noexcept {
  try {
    const std::string source = markdown ? markdown : "";
    const std::string theme = theme_id ? theme_id : "github";
    const std::string html = mtx::render_html(source, mtx::html_theme_from_id(theme));
    const std::size_t required = html.size() + 1;
    if (output && capacity >= required) {
      std::memcpy(output, html.c_str(), required);
    }
    return required;
  } catch (...) {
    if (output && capacity) output[0] = '\0';
    return 0;
  }
}

MDRAFT_API std::size_t mdraft_count_blocks(const char* markdown) noexcept {
  try {
    return mtx::parse_blocks(markdown ? std::string(markdown) : std::string()).size();
  } catch (...) {
    return 0;
  }
}
