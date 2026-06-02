# CMake/Windows ABI

v0.6.5 macht CMake zum primären Windows-Buildpfad.

## Targets

- `mdraft_core`: GapBuffer, Markdown, HTML, CommandManager
- `mdraft_io`: Datei-ABI
- `mdraft_ui_model`: Cursor, Viewport, Selection, Menu, Input-Actions
- `mdraft_runtime`: Save/Export/Undo/Redo/Clipboard-Kopplung
- `mdraft-win32`: native Win32/GDI-Projektion
- `mdraft-cli`: CLI-Fallback

Die Win32-GUI linkt nur gegen native System-ABIs:

- user32
- gdi32

Keine WebView, kein Electron, kein CEF, kein Qt.
