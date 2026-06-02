#pragma once
#include <string>

namespace mtx {

// External clipboard ABI. Implemented by native projection layers.
// Runtime owns editor semantics; OS adapters own platform transfer mechanics.
class ClipboardBridge {
public:
  virtual ~ClipboardBridge() = default;
  virtual bool read_text(std::string& out_utf8) = 0;
  virtual bool write_text(const std::string& utf8) = 0;
};

} // namespace mtx
