cmake_minimum_required(VERSION 3.21)

project(mdraft_native_cpp
  VERSION 0.12.3
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(MDRAFT_BUILD_TESTS "Build MDraft tests" ON)

if(MSVC)
  add_compile_options(/W4 /WX /permissive- /Zc:__cplusplus /utf-8 /EHsc /DNOMINMAX /DWIN32_LEAN_AND_MEAN)
else()
  add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

set(MDRAFT_INCLUDE_DIRS
  ${CMAKE_CURRENT_SOURCE_DIR}/core/include
  ${CMAKE_CURRENT_SOURCE_DIR}/io/include
  ${CMAKE_CURRENT_SOURCE_DIR}/ui_model/include
  ${CMAKE_CURRENT_SOURCE_DIR}/runtime/include
  ${CMAKE_CURRENT_SOURCE_DIR}/workspace/include
)

set(MDRAFT_CORE_SOURCES
  core/src/gap_buffer.cpp
  core/src/markdown.cpp
  core/src/html.cpp
  core/src/render_ir.cpp
  core/src/layout_engine.cpp
  core/src/render_svg.cpp
  core/src/command_manager.cpp
)

set(MDRAFT_IO_SOURCES
  io/src/file_io.cpp
)

set(MDRAFT_UI_MODEL_SOURCES
  ui_model/src/cursor_controller.cpp
  ui_model/src/viewport_manager.cpp
  ui_model/src/highlight_processor.cpp
  ui_model/src/command_dispatcher.cpp
  ui_model/src/glyph_metrics_table.cpp
  ui_model/src/input_dispatcher.cpp
  ui_model/src/command_registry.cpp
  ui_model/src/menu_bar.cpp
  ui_model/src/selection_engine.cpp
  ui_model/src/cursor_manager.cpp
  ui_model/src/context_projection.cpp
)

set(MDRAFT_RUNTIME_SOURCES
  runtime/src/editor_runtime.cpp
)

set(MDRAFT_WORKSPACE_SOURCES
  workspace/src/workspace.cpp
  workspace/src/file_index.cpp
  workspace/src/markdown_symbols.cpp
  workspace/src/link_resolver.cpp
  workspace/src/diagnostics.cpp
  workspace/src/workspace_search.cpp
  workspace/src/workspace_symbol_nav.cpp
  workspace/src/link_validation.cpp
  workspace/src/image_suggestions.cpp
  workspace/src/workspace_lint.cpp
  workspace/src/git_adapter.cpp
  workspace/src/snippets.cpp
)

add_library(mdraft_core STATIC ${MDRAFT_CORE_SOURCES})
target_include_directories(mdraft_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/core/include)

add_library(mdraft_io STATIC ${MDRAFT_IO_SOURCES})
target_include_directories(mdraft_io PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/io/include)

add_library(mdraft_ui_model STATIC ${MDRAFT_UI_MODEL_SOURCES})
target_include_directories(mdraft_ui_model PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/ui_model/include
  ${CMAKE_CURRENT_SOURCE_DIR}/core/include
)

add_library(mdraft_runtime STATIC ${MDRAFT_RUNTIME_SOURCES})
target_include_directories(mdraft_runtime PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/runtime/include
  ${CMAKE_CURRENT_SOURCE_DIR}/core/include
  ${CMAKE_CURRENT_SOURCE_DIR}/io/include
  ${CMAKE_CURRENT_SOURCE_DIR}/ui_model/include
)
target_link_libraries(mdraft_runtime PUBLIC mdraft_core mdraft_io mdraft_ui_model)

add_library(mdraft_workspace STATIC ${MDRAFT_WORKSPACE_SOURCES})
target_include_directories(mdraft_workspace PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/workspace/include
  ${CMAKE_CURRENT_SOURCE_DIR}/io/include
)
target_link_libraries(mdraft_workspace PUBLIC mdraft_io)

add_executable(mdraft-cli
  app/src/main.cpp
)
target_include_directories(mdraft-cli PRIVATE
  ${MDRAFT_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}/term/include
)
target_link_libraries(mdraft-cli PRIVATE mdraft_core mdraft_io)

if(WIN32)
  add_executable(mdraft-win32 WIN32
    gui_win32/src/native_menu_win32.cpp
    gui_win32/src/native_window_win32.cpp
    gui_win32/src/render_gdi_win32.cpp
    gui_win32/src/win32_clipboard_bridge.cpp
    app/src/gui_win32_main.cpp
  )
  target_include_directories(mdraft-win32 PRIVATE
    ${MDRAFT_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/gui_win32/include
  )
  target_compile_definitions(mdraft-win32 PRIVATE
    UNICODE
    _UNICODE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
  )
  target_link_libraries(mdraft-win32 PRIVATE
    mdraft_core
    mdraft_io
    mdraft_ui_model
    mdraft_runtime
    mdraft_workspace
    user32
    gdi32
    shell32
    comdlg32
    gdiplus
  )

  install(TARGETS mdraft-win32 RUNTIME DESTINATION .)
  install(TARGETS mdraft-cli RUNTIME DESTINATION bin)
  install(FILES README.md ARCHITECTURE.md USER_GUIDE.md CHANGELOG.md FEATURE_MATRIX.md PACKAGE_NOTE_PRODUCT_CONSOLIDATION.txt PACKAGE_NOTE_MDRAFT_RENAME.txt DESTINATION .)

  set(CPACK_GENERATOR "WIX")
  set(CPACK_PACKAGE_NAME "MDraft")
  set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Native Markdown Workspace Editor")
  set(CPACK_PACKAGE_VENDOR "Ralf Krümmel")
  set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
  set(CPACK_PACKAGE_INSTALL_DIRECTORY "MDraft")
  set(CPACK_WIX_INSTALL_SCOPE "perMachine")
  set(CPACK_WIX_PROGRAM_MENU_FOLDER ".")
  set(CPACK_PACKAGE_EXECUTABLES "mdraft-win32" "MDraft")
  set(CPACK_WIX_UPGRADE_GUID "B4FD8460-2E0A-4DFD-A32F-6F22B5223CE9")
  include(CPack)
endif()

if(UNIX AND NOT APPLE)
  find_package(X11)
  if(X11_FOUND)
    add_executable(mdraft-x11
      gui_x11/src/native_window_x11.cpp
      app/src/gui_x11_main.cpp
    )
    target_include_directories(mdraft-x11 PRIVATE
      ${MDRAFT_INCLUDE_DIRS}
      ${CMAKE_CURRENT_SOURCE_DIR}/gui_x11/include
      ${X11_INCLUDE_DIR}
    )
    target_link_libraries(mdraft-x11 PRIVATE
      mdraft_core
      mdraft_io
      mdraft_ui_model
      mdraft_runtime
      ${X11_LIBRARIES}
    )
  endif()
endif()

if(MDRAFT_BUILD_TESTS)
  enable_testing()

  add_executable(test_core
    core/test/test_core.cpp
  )
  target_link_libraries(test_core PRIVATE mdraft_core)
  add_test(NAME core COMMAND test_core)

  add_executable(test_io
    io/test/test_io.cpp
  )
  target_link_libraries(test_io PRIVATE mdraft_io)
  add_test(NAME io COMMAND test_io)

  add_executable(test_ui_model
    ui_model/test/test_ui_model.cpp
  )
  target_link_libraries(test_ui_model PRIVATE mdraft_ui_model mdraft_core)
  add_test(NAME ui_model COMMAND test_ui_model)

  add_executable(test_runtime
    runtime/test/test_runtime.cpp
  )
  target_link_libraries(test_runtime PRIVATE mdraft_runtime)
  add_test(NAME runtime COMMAND test_runtime)

  add_executable(test_workspace
    workspace/test/test_workspace.cpp
  )
  target_link_libraries(test_workspace PRIVATE mdraft_workspace)
  add_test(NAME workspace COMMAND test_workspace)
endif()
