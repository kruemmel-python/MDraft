# CMake/Windows ABI

v0.12.5 nutzt CMake als primären Windows-Buildpfad für MDraft.

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
- shell32
- comdlg32
- gdiplus

Keine WebView, kein Electron, kein CEF, kein Qt.
