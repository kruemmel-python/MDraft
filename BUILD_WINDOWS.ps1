$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot
$CMakeExe = if ($env:CMAKE_EXE) { $env:CMAKE_EXE } elseif (Test-Path "C:\Program Files\CMake\bin\cmake.exe") { "C:\Program Files\CMake\bin\cmake.exe" } else { "cmake.exe" }
$CTestExe = if ($env:CTEST_EXE) { $env:CTEST_EXE } elseif (Test-Path "C:\Program Files\CMake\bin\ctest.exe") { "C:\Program Files\CMake\bin\ctest.exe" } else { "ctest.exe" }
Remove-Item -Recurse -Force build_vs -ErrorAction SilentlyContinue
& $CMakeExe -S . -B build_vs -G "Visual Studio 17 2022" -A x64 -DMDRAFT_BUILD_TESTS=ON
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& $CMakeExe --build build_vs --config Release --target mdraft-win32 --parallel
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& $CMakeExe --build build_vs --config Release --target test_core test_io test_ui_model test_runtime test_workspace --parallel
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& $CTestExe --test-dir build_vs -C Release --output-on-failure
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
Write-Host "Fertig: build_vs\Release\mdraft-win32.exe"
