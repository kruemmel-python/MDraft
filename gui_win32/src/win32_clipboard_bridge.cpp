#include "mtx/win32_clipboard_bridge.hpp"

#include <string>

namespace mtx {
namespace {

std::wstring widen_utf8(const std::string& s) {
  if (s.empty()) {
    return {};
  }
  const int needed = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                         s.data(), static_cast<int>(s.size()),
                                         nullptr, 0);
  if (needed <= 0) {
    return {};
  }
  std::wstring out(static_cast<std::size_t>(needed), L'\0');
  const int written = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                          s.data(), static_cast<int>(s.size()),
                                          out.data(), needed);
  if (written <= 0) {
    return {};
  }
  return out;
}

std::string narrow_utf8(const wchar_t* text) {
  if (text == nullptr || *text == L'\0') {
    return {};
  }
  const int needed = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
  if (needed <= 1) {
    return {};
  }
  std::string out(static_cast<std::size_t>(needed - 1), '\0');
  const int written = WideCharToMultiByte(CP_UTF8, 0, text, -1, out.data(), needed, nullptr, nullptr);
  if (written <= 0) {
    return {};
  }
  return out;
}

} // namespace

Win32ClipboardBridge::Win32ClipboardBridge(HWND owner) noexcept : owner_(owner) {}

bool Win32ClipboardBridge::read_text(std::string& out_utf8) {
  out_utf8.clear();

  if (!IsClipboardFormatAvailable(CF_UNICODETEXT)) {
    return false;
  }
  if (OpenClipboard(owner_) == 0) {
    return false;
  }

  bool ok = false;
  HANDLE handle = GetClipboardData(CF_UNICODETEXT);
  if (handle != nullptr) {
    const wchar_t* locked = static_cast<const wchar_t*>(GlobalLock(handle));
    if (locked != nullptr) {
      out_utf8 = narrow_utf8(locked);
      GlobalUnlock(handle);
      ok = !out_utf8.empty();
    }
  }

  CloseClipboard();
  return ok;
}

bool Win32ClipboardBridge::write_text(const std::string& utf8) {
  const std::wstring wide = widen_utf8(utf8);
  if (utf8.empty()) {
    return false;
  }
  if (wide.empty()) {
    return false;
  }
  if (OpenClipboard(owner_) == 0) {
    return false;
  }

  bool ok = false;
  if (EmptyClipboard() != 0) {
    const SIZE_T bytes = (wide.size() + 1U) * sizeof(wchar_t);
    HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (memory != nullptr) {
      void* locked = GlobalLock(memory);
      if (locked != nullptr) {
        wchar_t* dst = static_cast<wchar_t*>(locked);
        for (std::size_t i = 0; i < wide.size(); ++i) {
          dst[i] = wide[i];
        }
        dst[wide.size()] = L'\0';
        GlobalUnlock(memory);

        if (SetClipboardData(CF_UNICODETEXT, memory) != nullptr) {
          memory = nullptr; // ownership transferred to the clipboard
          ok = true;
        }
      }
      if (memory != nullptr) {
        GlobalFree(memory);
      }
    }
  }

  CloseClipboard();
  return ok;
}

} // namespace mtx
