ifeq ($(OS),Windows_NT)
  EXE = .exe
  PLATFORM = windows
  TEST_PREFIX = .\\build\\
  ifeq ($(origin CXX), default)
    ifneq ($(wildcard C:/Progra~1/LLVM/bin/clang++.exe),)
      CXX = C:/Progra~1/LLVM/bin/clang++.exe
    else
      CXX = g++
    endif
  endif
else
  EXE =
  PLATFORM = linux
  TEST_PREFIX = ./build/
  ifeq ($(origin CXX), default)
    CXX = g++
  endif
endif

CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -O2

INCLUDES = -Icore/include -Iio/include -Iui_model/include -Iruntime/include -Iworkspace/include -Igui_x11/include -Igui_win32/include

CORE = core/src/gap_buffer.cpp core/src/markdown.cpp core/src/html.cpp core/src/render_ir.cpp core/src/layout_engine.cpp core/src/render_svg.cpp core/src/command_manager.cpp
IO = io/src/file_io.cpp
UI_MODEL = ui_model/src/cursor_controller.cpp ui_model/src/viewport_manager.cpp ui_model/src/highlight_processor.cpp ui_model/src/command_dispatcher.cpp ui_model/src/glyph_metrics_table.cpp ui_model/src/input_dispatcher.cpp ui_model/src/command_registry.cpp ui_model/src/menu_bar.cpp ui_model/src/selection_engine.cpp ui_model/src/cursor_manager.cpp ui_model/src/context_projection.cpp
RUNTIME = runtime/src/editor_runtime.cpp
WORKSPACE = workspace/src/workspace.cpp workspace/src/file_index.cpp workspace/src/markdown_symbols.cpp workspace/src/link_resolver.cpp workspace/src/diagnostics.cpp workspace/src/workspace_search.cpp workspace/src/workspace_symbol_nav.cpp workspace/src/link_validation.cpp workspace/src/image_suggestions.cpp workspace/src/workspace_lint.cpp workspace/src/git_adapter.cpp workspace/src/snippets.cpp
GUI_X11 = gui_x11/src/native_window_x11.cpp
GUI_WIN32 = gui_win32/src/native_menu_win32.cpp gui_win32/src/native_window_win32.cpp gui_win32/src/render_gdi_win32.cpp gui_win32/src/win32_clipboard_bridge.cpp

.PHONY: all gui gui-x11 gui-win32 test clean platform

all: build/mdraft$(EXE)

platform:
	@echo "platform=$(PLATFORM)"

build:
	mkdir -p build

build/mdraft$(EXE): build $(CORE) $(IO) app/src/main.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(CORE) $(IO) app/src/main.cpp -o $@

gui:
ifeq ($(OS),Windows_NT)
	$(MAKE) gui-win32
else
	$(MAKE) gui-x11
endif

gui-x11: build/mdraft-x11$(EXE)

build/mdraft-x11$(EXE): build $(CORE) $(IO) $(UI_MODEL) $(RUNTIME) $(WORKSPACE) $(GUI_X11) app/src/gui_x11_main.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(CORE) $(IO) $(UI_MODEL) $(RUNTIME) $(WORKSPACE) $(GUI_X11) app/src/gui_x11_main.cpp -lX11 -o $@

gui-win32: build/mdraft-win32.exe

build/mdraft-win32.exe: build $(CORE) $(IO) $(UI_MODEL) $(RUNTIME) $(WORKSPACE) $(GUI_WIN32) app/src/gui_win32_main.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(CORE) $(IO) $(UI_MODEL) $(RUNTIME) $(WORKSPACE) $(GUI_WIN32) app/src/gui_win32_main.cpp -lgdi32 -luser32 -lshell32 -lcomdlg32 -lgdiplus -o $@

build/test_core$(EXE): build $(CORE) core/test/test_core.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(CORE) core/test/test_core.cpp -o $@

build/test_io$(EXE): build $(IO) io/test/test_io.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(IO) io/test/test_io.cpp -o $@

build/test_ui_model$(EXE): build $(CORE) $(UI_MODEL) ui_model/test/test_ui_model.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(CORE) $(UI_MODEL) ui_model/test/test_ui_model.cpp -o $@

build/test_runtime$(EXE): build $(CORE) $(IO) $(UI_MODEL) $(RUNTIME) runtime/test/test_runtime.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(CORE) $(IO) $(UI_MODEL) $(RUNTIME) runtime/test/test_runtime.cpp -o $@

build/test_workspace$(EXE): build $(IO) $(WORKSPACE) workspace/test/test_workspace.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(IO) $(WORKSPACE) workspace/test/test_workspace.cpp -o $@

test: build/test_core$(EXE) build/test_io$(EXE) build/test_ui_model$(EXE) build/test_runtime$(EXE) build/test_workspace$(EXE)
	$(TEST_PREFIX)test_core$(EXE)
	$(TEST_PREFIX)test_io$(EXE)
	$(TEST_PREFIX)test_ui_model$(EXE)
	$(TEST_PREFIX)test_runtime$(EXE)
	$(TEST_PREFIX)test_workspace$(EXE)
	@echo "all tests ok"

clean:
	rm -rf build
