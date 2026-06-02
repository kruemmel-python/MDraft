#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "mtx/command_registry.hpp"
#include "mtx/editor_runtime.hpp"

#include <windows.h>
#include <cstdint>

namespace mtx {

// Win32-native menu adapter.
//
// The menu is an OS object (HMENU), not painted text. It is deliberately only
// an adapter: it maps Win32 command integers to the shared CommandID substrate.
// Runtime and core behavior remain outside the GUI layer.
class NativeMenuWin32 {
public:
  NativeMenuWin32() = default;
  ~NativeMenuWin32();

  NativeMenuWin32(const NativeMenuWin32&) = delete;
  NativeMenuWin32& operator=(const NativeMenuWin32&) = delete;

  void attach(HWND hwnd, const CommandRegistry& registry);
  CommandID command_from_wparam(WPARAM wp) const noexcept;
  void sync_enabled(const EditorRuntime& runtime) noexcept;
  void set_checked(CommandID id, bool checked) noexcept;

private:
  HWND hwnd_{nullptr};
  HMENU root_{nullptr};

  static UINT win_id(CommandID id) noexcept;
  static CommandID command_id(UINT id) noexcept;
  static void append_command(HMENU menu,
                             const CommandRegistry& registry,
                             CommandID id,
                             const wchar_t* label);
  static void append_separator(HMENU menu);
  void enable(CommandID id, bool enabled) noexcept;
};

} // namespace mtx
