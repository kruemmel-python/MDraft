# MDraft
<img width="1920" height="1080" alt="image" src="https://github.com/user-attachments/assets/d41986bb-ec08-4a8a-a06d-22fef90c6fcf" />

**Native Markdown Workspace Editor**  
**Developer:** Ralf Krümmel

**MDraft v0.12.4**  
Codename: **Workspace Snippets Images**  
Status: **Developer Preview / Stable Core**

MDraft ist ein nativer Markdown- und Workspace-Editor mit eigener RenderIR-Pipeline. Der Editor kombiniert eine hardwarenahe Win32-UI, Live-Preview, Theme-Export, Projektindex, Symbolnavigation, Linkprüfung, Diagnostics/Linting, Git-Status light, Snippets und Bildpfad-Unterstützung.

MDraft ist kein Web-Wrapper und keine CRUD-Anwendung. Der Kern ist ein Substrat:

```text
Editor Buffer
  -> Markdown / RenderIR
  -> Live Preview
  -> HTML/SVG Export

Workspace Root
  -> FileIndex
  -> Symbols
  -> Links
  -> Assets
  -> Diagnostics
  -> Search / Navigation / Suggestions
```

## Aktueller Funktionsumfang

- Nativer Markdown-Editor mit Win32-Menü und Tastatursteuerung
- RenderIR-basierte Live-Preview
- Preview-Themen: Standard, Cyberpunk, Dystopie, Horror, Abenteuer
- Preview-Locking
- Preview-Scroll
- HTML/SVG-Export über dieselbe RenderIR-Schicht
- Markdown-Tabellen, Mathe-Subset, Mermaid-Subset
- Bilder im RenderIR: Markdown `![alt](path)` -> Preview + SVG/HTML Export
- Snippets: Überschriften, Todo, Tabellen, Mathe, Mermaid, Code, Bilder, Notizen
- Workspace öffnen und neu indizieren
- Multi-Datei-Suche
- Symbol-Navigation
- Link-Validierung
- Bildpfad-Vorschläge
- Diagnostics/Linting
- Git-Status light über `git.exe`
- Windows MSI über CPack/WiX

## Projektstruktur

```text
core/
  Markdown, RenderIR, Layout, HTML/SVG Export, CommandManager

io/
  Dateizugriff, atomisches Schreiben, Windows UTF-16 Pfadschicht

ui_model/
  Eingabe, Menüs, Cursor, Viewport, Auswahl, Syntax-Highlighting

runtime/
  EditorRuntime, Datei-Workflow, Clipboard, Export-Befehle

workspace/
  FileIndex, Symbols, Links, Search, Diagnostics, Git, Snippets, Images

gui_win32/
  Native Win32 Window, Menü, Preview-Renderer, GDI/GDI+ Bildanzeige
```

## Build unter Windows

```powershell
cd D:\marktex_workspace_search_v0102

$env:CMAKE_EXE = "C:\Program Files\CMake\bin\cmake.exe"
$env:CTEST_EXE = "C:\Program Files\CMake\bin\ctest.exe"
$env:CPACK_EXE = "C:\Program Files\CMake\bin\cpack.exe"

.\BUILD_WINDOWS.ps1
```

MSI:

```powershell
.\BUILD_MSI_WINDOWS.ps1
```

## Build mit Make

```bash
make clean
make test
make all
```

## Tests

```text
core
io
ui_model
runtime
workspace
```

Der Workspace-Test prüft Indexing, Symbols, Links, Search, Symbol Navigation, Link Validation, Image Suggestions, Linting, Git-Fallbacks und Snippets.

## Dokumentation

- [`ARCHITECTURE.md`](ARCHITECTURE.md)
- [`USER_GUIDE.md`](USER_GUIDE.md)
- [`FEATURE_MATRIX.md`](FEATURE_MATRIX.md)


## Entwicklungsprinzip

```text
Substrat vor Oberfläche
Runtime vor UI
Test vor Behauptung
ABI vor Komfort
Determinismus vor Begeisterung
```
