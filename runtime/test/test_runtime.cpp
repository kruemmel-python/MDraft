#include "mtx/editor_runtime.hpp"
#include "mtx/file_io.hpp"
#include <cassert>
#include <cstdio>
#include <exception>
#include <iostream>
#include <string>

namespace {
class FakeClipboard final : public mtx::ClipboardBridge {
public:
  bool read_text(std::string& out_utf8) override {
    out_utf8 = text;
    return !text.empty();
  }
  bool write_text(const std::string& utf8) override {
    text = utf8;
    return true;
  }
  std::string text;
};
} // namespace

int main() {
  try {
    // CTest runs Visual-Studio builds from build_vs_msi, not from the source root.
    // The old path "build/mdraft_runtime_test.md" assumed a nested build/
    // directory that does not exist under build_vs_msi and therefore tripped
    // write_file_atomic on Windows. Keep test artefacts in the current writable
    // CTest working directory.
    const std::string path = "mdraft_runtime_test.md";
    mtx::GapBuffer buffer("abc");
    mtx::EditorState state;
    state.cursor_byte = 3;
    mtx::CommandManager commands;
    mtx::EditorRuntime rt(buffer, state, commands, path);
    FakeClipboard fake_clipboard;
    rt.set_clipboard_bridge(&fake_clipboard);

    std::remove(path.c_str());
    std::remove((path + ".html").c_str());
    std::remove((path + ".cyberpunk.html").c_str());

    rt.insert_text("d");
    assert(buffer.str() == "abcd");
    assert(state.dirty);

    bool running = true;
    assert(rt.execute(mtx::CommandID::Save, running));
    assert(running);
    assert(!state.dirty);
    assert(mtx::read_file(path) == "abcd");

    rt.execute(mtx::CommandID::SelectAll, running);
    assert(state.selection.active);
    rt.execute(mtx::CommandID::Copy, running);
    assert(rt.clipboard() == "abcd");
    assert(fake_clipboard.text == "abcd");
    rt.execute(mtx::CommandID::Cut, running);
    assert(buffer.str().empty());
    rt.execute(mtx::CommandID::Paste, running);
    assert(buffer.str() == "abcd");

    fake_clipboard.text = " EXTERN";
    state.cursor_byte = buffer.size();
    rt.execute(mtx::CommandID::Paste, running);
    assert(buffer.str() == "abcd EXTERN");

    rt.execute(mtx::CommandID::ExportHtml, running);
    const std::string html = mtx::read_file(path + ".html");
    assert(html.find("<!doctype html>") != std::string::npos);
    assert(html.find("MDraft RenderIR Export") != std::string::npos);

    rt.execute(mtx::CommandID::ExportHtmlCyberpunk, running);
    const std::string cyber = mtx::read_file(path + ".cyberpunk.html");
    assert(cyber.find("theme-cyberpunk") != std::string::npos);

    const std::string save_as_path = "mdraft_runtime_test_save_as.md";
    rt.save_as(save_as_path);
    assert(rt.path() == save_as_path);
    assert(mtx::read_file(save_as_path) == "abcd EXTERN");

    rt.open_document(path, "opened");
    assert(rt.path() == path);
    assert(buffer.str() == "opened");
    assert(!state.dirty);

    rt.new_document();
    assert(rt.path().empty());
    assert(buffer.str().empty());
    assert(!state.dirty);

    std::remove(path.c_str());
    std::remove(save_as_path.c_str());
    std::remove((path + ".html").c_str());
    std::cout << "runtime ok\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "runtime test failed: " << e.what() << "\n";
    return 2;
  }
}
