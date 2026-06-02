#pragma once
#include "mtx/command_registry.hpp"
#include <string>
#include <vector>

namespace mtx {

struct MenuCell {
  int x{0};
  int y{0};
  int w{0};
  int h{0};
  CommandID command{CommandID::NoCommand};
  std::string label;
  std::string shortcut;
};

class MenuBar {
public:
  explicit MenuBar(int height = 28);

  int height() const noexcept { return height_; }
  void rebuild(const CommandRegistry& registry, int char_w);
  bool contains(int x, int y) const noexcept;
  CommandID command_at(int x, int y) const noexcept;
  const std::vector<MenuCell>& cells() const noexcept { return cells_; }

private:
  int height_{28};
  std::vector<MenuCell> cells_;
};

} // namespace mtx
