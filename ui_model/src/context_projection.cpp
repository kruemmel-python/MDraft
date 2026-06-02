#include "mtx/context_projection.hpp"

namespace mtx {

std::string ContextProjection::token_label(const std::string& text, std::size_t byte, const AttributeBuffer& attrs) const {
  if (text.empty()) return "Context: empty document";
  if (byte >= text.size()) byte = text.size() - 1;
  const TextStyle s = attrs.at(byte);
  if (has_style(s, TextStyle::Heading1) || has_style(s, TextStyle::Heading2) || has_style(s, TextStyle::Heading3)) return "Context: heading";
  if (has_style(s, TextStyle::Code)) return "Context: code span/block";
  if (has_style(s, TextStyle::Math)) return "Context: math";
  if (has_style(s, TextStyle::Quote)) return "Context: quote";
  if (has_style(s, TextStyle::List)) return "Context: list/task item";
  if (text[byte] == '|') return "Context: table cell candidate";
  return "Context: plain text";
}

} // namespace mtx
