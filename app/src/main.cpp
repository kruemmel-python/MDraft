#include "mtx/file_io.hpp"
#include "mtx/gap_buffer.hpp"
#include "mtx/html.hpp"
#include <iostream>
#include <string>

int main(int argc, char** argv) {
  std::string path = "examples/demo.md";
  mtx::HtmlTheme theme = mtx::HtmlTheme::Horror;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i] ? argv[i] : "";
    if (arg == "--theme" && i + 1 < argc) {
      theme = mtx::html_theme_from_id(argv[++i] ? argv[i] : "");
    } else if (arg.rfind("--theme=", 0) == 0) {
      theme = mtx::html_theme_from_id(arg.substr(8));
    } else if (arg == "--help" || arg == "-h") {
      std::cout << "usage: mdraft [file.md] [--theme standard|cyberpunk|dystopie|horror|abenteuer]\n";
      return 0;
    } else {
      path = arg;
    }
  }

  mtx::GapBuffer b(mtx::read_file(path));
  std::cout << "MDraft CLI core check\n";
  std::cout << "bytes=" << b.size() << "\n";
  std::cout << "html export: " << path << ".html\n";
  std::cout << "theme=" << mtx::html_theme_label(theme) << "\n";
  mtx::write_file_atomic(path + ".html", mtx::render_html(b.str(), theme));
  return 0;
}
