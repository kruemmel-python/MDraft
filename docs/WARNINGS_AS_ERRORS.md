# Warnings-as-Errors ABI

v0.6.5 treats compiler warnings as build failures.

## MSVC

CMake applies:

```text
/W4 /WX /permissive- /Zc:__cplusplus /utf-8 /EHsc /DNOMINMAX /DWIN32_LEAN_AND_MEAN
```

## GCC/Clang

CMake applies:

```text
-Wall -Wextra -Wpedantic -Werror
```

## Fixed in v0.6.5

- `ui_model/test/test_ui_model.cpp`: the context projection test no longer relies on an `assert(...)` expression in Release mode. The result is now checked by executable runtime logic, so MSVC no longer sees the local `cp` object as unused.
- `gui_win32/src/native_window_win32.cpp`: Win32 `TEXTMETRICW` fields are converted explicitly to `int` before range clamping. This removes the ambiguous `std::max(LONG, int)` overload failure under MSVC.
