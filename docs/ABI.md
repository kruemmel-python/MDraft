# ABI-Grenzen

```text
/core
  GapBuffer
  Markdown parser
  HTML exporter

/io
  read_file
  write_file_atomic

/gui_x11
  EditorState
  CursorController
  ViewportManager
  HighlightProcessor
  CommandDispatcher
  NativeWindowX11

/app
  koppelt Datei, Buffer und Fenster
```

Keine GUI-Struktur besitzt Dokumentinhalt.
Die GUI besitzt nur Cursor-, Selection-, Viewport- und Render-Zustand.
