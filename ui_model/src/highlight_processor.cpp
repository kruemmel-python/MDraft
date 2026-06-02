#include "mtx/highlight_processor.hpp"
#include <algorithm>

namespace mtx {

AttributeBuffer::AttributeBuffer(std::size_t n) : attrs_(n, TextStyle::Normal) {}
void AttributeBuffer::reset(std::size_t n) { attrs_.assign(n, TextStyle::Normal); }

void AttributeBuffer::apply(std::size_t begin, std::size_t end, TextStyle style) {
  if (begin > end) std::swap(begin, end);
  end = std::min(end, attrs_.size());
  for (std::size_t i = begin; i < end; ++i) attrs_[i] |= style;
}

TextStyle AttributeBuffer::at(std::size_t i) const {
  return i < attrs_.size() ? attrs_[i] : TextStyle::Normal;
}

AttributeBuffer HighlightProcessor::build(const std::string& text, const Selection& selection) const {
  AttributeBuffer out(text.size());
  for (const StyledSpan& s : parse_inline_styles(text)) out.apply(s.begin, s.end, s.style);
  if (selection.active && selection.begin() != selection.end()) out.apply(selection.begin(), selection.end(), TextStyle::Selection);
  return out;
}

} // namespace mtx
