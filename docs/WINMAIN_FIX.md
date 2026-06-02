# Win32 Entry-Point ABI Fix v0.6.6

`mdraft-win32` is intentionally built as a Windows GUI subsystem target.

CMake uses:

```cmake
add_executable(mdraft-win32 WIN32 ...)
```

MSVC therefore requires a Win32 entry point (`WinMain`/`wWinMain`) instead of the console `main` entry point. v0.6.6 provides:

```cpp
int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
```

Command-line arguments are decoded through `CommandLineToArgvW` and converted to UTF-8 before entering the editor runtime.

The GUI target links only native OS libraries:

```text
user32
gdi32
shell32
```

No Qt, CEF, Electron, WebView, or browser runtime is used.
