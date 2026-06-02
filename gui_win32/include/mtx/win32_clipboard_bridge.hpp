#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "mtx/clipboard_bridge.hpp"
#include <windows.h>

namespace mtx {

// Native Windows clipboard adapter.
// Transfers text as CF_UNICODETEXT and converts at the ABI boundary to UTF-8.
class Win32ClipboardBridge final : public ClipboardBridge {
public:
  explicit Win32ClipboardBridge(HWND owner) noexcept;

  bool read_text(std::string& out_utf8) override;
  bool write_text(const std::string& utf8) override;

private:
  HWND owner_{nullptr};
};

} // namespace mtx
