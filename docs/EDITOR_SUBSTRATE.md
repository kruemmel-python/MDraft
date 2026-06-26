# MarkTeX v0.3 Editor-Substrat

## EditorState

`EditorState` hält ausschließlich UI-Zustand:

- `cursor_byte`
- `preferred_col`
- `scroll_row`
- `scroll_col`
- `selection.anchor/focus`
- `mode`
- `dirty/status`

Der Dokumentinhalt bleibt im `GapBuffer`.

## CursorController

Wandelt `byte_index <-> row/col` über einen linearen `LineIndex`.
Dadurch wird der Cursor im Dokument gezeichnet und nicht nur als Statuswert angezeigt.

## ViewportManager

Berechnet sichtbare Zeilen und Spalten aus Pixelgröße und Zellmetrik.
Scrolling mutiert nur `EditorState::scroll_row/scroll_col`.

## HighlightProcessor

Transformiert Markdown-Block/Inline-Spannen in einen `AttributeBuffer`.
Der Renderer liest Attribute pro Byte und entscheidet daraus Vordergrund/Hintergrund/Stil.

## CommandDispatcher

Trennt Eingabetext von Befehlen:

- Ctrl-S: Save
- Ctrl-E: HTML export
- Ctrl-Q: Quit
- Escape: Insert/Command-Modus
- Textmutation nur im Insert-Modus
