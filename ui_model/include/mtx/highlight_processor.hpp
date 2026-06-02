#pragma once
#include "mtx/markdown.hpp"
#include "mtx/editor_state.hpp"
#include <cstddef>
#include <string>
#include <vector>

namespace mtx {

class AttributeBuffer {
public:
  explicit AttributeBuffer(std::size_t n = 0);
  void reset(std::size_t n);
  void apply(std::size_t begin, std::size_t end, TextStyle style);
  TextStyle at(std::size_t i) const;
  std::size_t size() const noexcept { return attrs_.size(); }
private:
  std::vector<TextStyle> attrs_;
};

class HighlightProcessor {
public:
  AttributeBuffer build(const std::string& text, const Selection& selection) const;
};

} // namespace mtx
