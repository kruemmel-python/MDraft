@echo off
setlocal
cd /d "%~dp0"
if "%CMAKE_EXE%"=="" set "CMAKE_EXE=C:\Program Files\CMake\bin\cmake.exe"
if "%CTEST_EXE%"=="" set "CTEST_EXE=C:\Program Files\CMake\bin\ctest.exe"
if exist build_vs rmdir /s /q build_vs
"%CMAKE_EXE%" -S . -B build_vs -G "Visual Studio 17 2022" -A x64 -DMDRAFT_BUILD_TESTS=ON || exit /b 1
"%CMAKE_EXE%" --build build_vs --config Release --target mdraft-win32 --parallel || exit /b 1
"%CMAKE_EXE%" --build build_vs --config Release --target test_core test_io test_ui_model test_runtime test_workspace --parallel || exit /b 1
"%CTEST_EXE%" --test-dir build_vs -C Release --output-on-failure || exit /b 1
echo Fertig: build_vs\Release\mdraft-win32.exe
endlocal
