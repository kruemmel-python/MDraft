#pragma once
#include "mtx/highlight_processor.hpp"
#include <cstddef>
#include <string>

namespace mtx {

class ContextProjection {
public:
  std::string token_label(const std::string& text, std::size_t byte, const AttributeBuffer& attrs) const;
};

} // namespace mtx
