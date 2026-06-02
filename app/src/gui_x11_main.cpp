#include "mtx/file_io.hpp"
#include "mtx/gap_buffer.hpp"
#include "mtx/native_window_x11.hpp"
#include "mtx/command_manager.hpp"
#include "mtx/editor_runtime.hpp"
#include "mtx/editor_state.hpp"
#include <iostream>

int main(int argc, char** argv) {
  const std::string path = argc > 1 ? argv[1] : "untitled.md";
  try {
    mtx::GapBuffer buffer(mtx::read_file(path));
    mtx::EditorState state;
    state.cursor_byte = buffer.size();
    mtx::CommandManager commands;
    mtx::EditorRuntime runtime(buffer, state, commands, path);
    mtx::NativeWindowX11 win(1100, 720);
    win.run(runtime);
  } catch (const std::exception& e) {
    std::cerr << "mdraft-x11: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
