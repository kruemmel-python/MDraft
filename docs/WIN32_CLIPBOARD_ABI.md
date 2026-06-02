# Win32 Clipboard ABI - v0.8

The editor no longer uses only an internal clipboard.

## Boundary

```text
Menu/Shortcut Paste
  -> CommandID::Paste
  -> EditorRuntime
  -> ClipboardBridge::read_text()
  -> CommandManager::insert()
  -> GapBuffer
```

```text
Copy/Cut
  -> EditorRuntime selection slice
  -> ClipboardBridge::write_text()
  -> internal fallback clipboard
```

## Win32 implementation

`gui_win32/src/win32_clipboard_bridge.cpp` transfers text as `CF_UNICODETEXT`.
The runtime remains UTF-8 internally. UTF-16 conversion happens only at the OS ABI boundary.

No Qt, no Electron, no CEF, no web runtime.

## Fallback

If the OS clipboard cannot be opened, runtime keeps the internal clipboard.
Status text marks fallback with `(intern)`.
