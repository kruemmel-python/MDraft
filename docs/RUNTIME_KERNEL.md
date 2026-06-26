# MarkTeX Native v0.6 Runtime Kernel

v0.6 removes application decisions from the X11 projection layer.

## ABI split

```text
core/       GapBuffer, parser, HTML export, edit command stack
io/         atomic file IO only
ui_model/   cursor, selection, viewport, highlighting, menus, input actions
runtime/    application command bus: save/export/clipboard/undo/redo/mutations
gui_x11/    X11 window, event source, renderer, pixel-to-byte projection
app/        bootstrapping and object ownership
```

## Event path

```text
X11 Event
  -> gui_x11 normalization
  -> ui_model InputDispatcher/MenuBar/SelectionEngine
  -> runtime EditorRuntime
  -> core/io mutation
  -> redraw invalidation
```

## Render path

```text
EditorRuntime snapshot
  -> GapBuffer string snapshot
  -> ui_model HighlightProcessor/Viewport/CursorManager
  -> gui_x11 XDrawString projection
```

The GUI may not call `write_file_atomic`, `render_html`, or `CommandManager` directly.
