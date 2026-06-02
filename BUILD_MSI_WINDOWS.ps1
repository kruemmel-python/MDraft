$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot
$CMakeExe = if ($env:CMAKE_EXE) { $env:CMAKE_EXE } elseif (Test-Path "C:\Program Files\CMake\bin\cmake.exe") { "C:\Program Files\CMake\bin\cmake.exe" } else { "cmake.exe" }
$CTestExe = if ($env:CTEST_EXE) { $env:CTEST_EXE } elseif (Test-Path "C:\Program Files\CMake\bin\ctest.exe") { "C:\Program Files\CMake\bin\ctest.exe" } else { "ctest.exe" }
$CPackExe = if ($env:CPACK_EXE) { $env:CPACK_EXE } elseif (Test-Path "C:\Program Files\CMake\bin\cpack.exe") { "C:\Program Files\CMake\bin\cpack.exe" } else { "cpack.exe" }
Remove-Item -Recurse -Force build_vs_msi -ErrorAction SilentlyContinue
& $CMakeExe -S . -B build_vs_msi -G "Visual Studio 17 2022" -A x64 -DMDRAFT_BUILD_TESTS=ON
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& $CMakeExe --build build_vs_msi --config Release --target mdraft-win32 mdraft-cli --parallel
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& $CMakeExe --build build_vs_msi --config Release --target test_core test_io test_ui_model test_runtime test_workspace --parallel
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& $CTestExe --test-dir build_vs_msi -C Release --output-on-failure
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
Push-Location build_vs_msi
& $CPackExe -G WIX -C Release
$code = $LASTEXITCODE
Pop-Location
if ($code -ne 0) { exit $code }
Get-ChildItem build_vs_msi\*.msi
