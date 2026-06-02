#include "mtx/native_window_win32.hpp"
#include "mtx/file_io.hpp"
#include "mtx/gap_buffer.hpp"
#include "mtx/command_manager.hpp"
#include "mtx/editor_state.hpp"
#include "mtx/editor_runtime.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>

#include <exception>
#include <string>

namespace {

std::string narrow_utf8(const wchar_t* text) {
  if (!text || *text == L'\0') {
    return {};
  }
  const int needed = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
  if (needed <= 1) {
    return {};
  }
  std::string out(static_cast<std::size_t>(needed - 1), '\0');
  WideCharToMultiByte(CP_UTF8, 0, text, -1, out.data(), needed, nullptr, nullptr);
  return out;
}

std::string input_path_from_command_line() {
  int argc = 0;
  LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
  std::string path;
  if (argv != nullptr) {
    if (argc > 1 && argv[1] != nullptr && argv[1][0] != L'\0') {
      path = narrow_utf8(argv[1]);
    }
    LocalFree(argv);
  }
  return path;
}

int run_mdraft_win32(const std::string& path) {
  mtx::GapBuffer buffer(mtx::read_file(path));
  mtx::EditorState state;
  state.cursor_byte = 0;

  mtx::CommandManager commands;
  mtx::EditorRuntime runtime(buffer, state, commands, path);

  mtx::NativeWindowWin32 win(1100, 720);
  return win.run(runtime) ? 0 : 1;
}

} // namespace

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
  try {
    return run_mdraft_win32(input_path_from_command_line());
  } catch (const std::exception& e) {
    MessageBoxA(nullptr, e.what(), "mdraft-win32", MB_OK | MB_ICONERROR);
    return 1;
  } catch (...) {
    MessageBoxA(nullptr, "unknown fatal error", "mdraft-win32", MB_OK | MB_ICONERROR);
    return 1;
  }
}
