# Win32 ABI Projection

`gui_win32/` is a native OS projection layer.

It owns:
- HWND lifecycle
- Win32 message normalization
- GDI text projection
- pixel-to-byte hit-testing
- invalidation

It must not own:
- document mutation policy
- save/export logic
- clipboard policy beyond OS event transport
- Markdown semantics

Path:

```text
WM_KEYDOWN/WM_CHAR/WM_MOUSE*
  -> gui_win32 normalized event
  -> ui_model InputDispatcher/SelectionEngine/ViewportManager
  -> runtime EditorRuntime
  -> core/io
  -> InvalidateRect
  -> draw snapshot
```

This mirrors the X11 path but replaces Xlib with Win32/GDI.
