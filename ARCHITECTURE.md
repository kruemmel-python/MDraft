# MDraft Architecture

**Product:** MDraft — Native Markdown Workspace Editor  
**Developer:** Ralf Krümmel

Version: **v0.12.5**
Codename: **Workspace Snippets Images**

## 1. Kern des Projekts

MDraft ist ein nativer Markdown-/Workspace-Editor, dessen zentrale Wahrheit nicht die UI ist, sondern ein Satz stabiler Substrate:

```text
Editor Buffer
Workspace Index
RenderIR / DisplayList
Diagnostics
Export Pipeline
```

Die UI projiziert diese Daten. Sie besitzt keine versteckte Businesslogik.

## 2. Warum Standardlösungen zu flach sind

Ein gewöhnlicher Markdown-Editor macht meist:

```text
Textfeld
  -> WebView Preview
  -> Export durch externe Library
```

MDraft macht bewusst:

```text
Markdown
  -> eigenes Parse/Layout-Modell
  -> RenderIR
  -> Win32/GDI Preview
  -> SVG/HTML Export

Workspace
  -> FileIndex
  -> Symbols / Links / Diagnostics
  -> Search / Navigation / Suggestions
```

Das ist tiefer, weil Preview und Export dieselbe Datenquelle teilen und Workspace-Funktionen nicht als UI-Gimmicks angeklebt werden.

## 3. Schichten

### core/

Zuständig für dokumentlokale Wahrheit:

```text
GapBuffer
Markdown-Hilfen
RenderIR
LayoutEngine
RenderSVG
HTML Theme Export
CommandManager
```

Wichtige Dateien:

```text
core/include/mtx/render_ir.hpp
core/include/mtx/layout_engine.hpp
core/include/mtx/render_svg.hpp
core/include/mtx/html.hpp
```

### io/

Zuständig für Dateisystem-ABI:

```text
read_file
write_file_atomic
Windows UTF-8 -> UTF-16 Pfade
CreateFileW / MoveFileExW
```

Die IO-Schicht wirft Exceptions mit konkreten Pfaden und Fehlercodes. Die UI fängt diese an der Command-Grenze ab.

### ui_model/

Zuständig für UI-unabhängige Interaktion:

```text
CommandRegistry
MenuBar Model
InputDispatcher
SelectionEngine
ViewportManager
SyntaxHighlighting
CursorManager
```

### runtime/

Zuständig für Editorzustand und Befehlsausführung:

```text
EditorRuntime
new_document
open_document
save_as
export_html_theme
clipboard bridge
```

Die Runtime kennt keine Win32-Dialoge. Dialoge sind Projektion der GUI-Schicht.

### workspace/

Zuständig für Projektkontext:

```text
Workspace
FileIndex
MarkdownSymbols
LinkResolver
LinkValidation
ImageSuggestions
WorkspaceSearch
WorkspaceSymbolNav
WorkspaceLint
GitAdapter
Snippets
Diagnostics
```

Das Workspace-Modul ist bewusst getrennt vom Editor. Dadurch kann derselbe Index später für Extension ABI, Export Pipelines und Projekt-Diagnostics genutzt werden.

### gui_win32/

Zuständig für native Windows-Projektion:

```text
NativeWindowWin32
NativeMenuWin32
RenderGDIWin32
Win32ClipboardBridge
GDI+ image drawing
```

## 4. RenderIR-Datenfluss

```text
Markdown
  -> markdown_to_display_list(markdown, theme, width)
  -> DisplayList
       Rect
       Line
       Text
       Image
  -> draw_display_list_gdi(...)
  -> display_list_to_svg_html(...)
```

Die Preview ist keine separate Skizze mehr. Sie ist eine GDI-Projektion derselben DisplayList, aus der der HTML/SVG Export entsteht.

## 5. Workspace-Datenfluss

```text
WorkspaceRoot
  -> scan_workspace(...)
  -> WorkspaceFile[]
  -> index_workspace(...)
  -> Symbols[]
  -> Links[]
  -> Assets[]
  -> Diagnostics[]
```

Darauf sitzen:

```text
Multi-Datei-Suche
Symbolnavigation
Link-Validierung
Bildpfad-Vorschläge
Linting
Git-Status light
Snippets
```

## 6. Diagnostics/Linting

Diagnostics sind Daten, nicht UI-Text:

```cpp
struct Diagnostic {
  std::string file;
  std::size_t byte_offset;
  DiagnosticSeverity severity;
  std::string message;
  std::size_t line;
  std::size_t column;
  std::string code;
  std::string fix_hint;
};
```

Aktuelle Lint-Regeln:

```text
MD001 trailing whitespace
MD002 tabs in Markdown text
MD003 long line
MD004 heading level jump
MD005 duplicate heading anchor
MD006 image without alt text
MD007 link without visible text
MD008 malformed pipe table shape
MD009 missing H1
MD010 multiple H1 headings
```

## 7. Git-Status light

Git ist aktuell ein Prozess-Adapter, keine LibGit2-Abhängigkeit:

```text
git -C <root> rev-parse --show-toplevel
git -C <root> status --porcelain=v1
```

Fehlermodi werden explizit modelliert:

```text
git.exe fehlt
kein Repository
clean working tree
modified / added / deleted / renamed / untracked / conflict
```

## 8. Extension-ABI-Vorbereitung

Die aktuelle Architektur bereitet eine zukünftige Extension ABI vor:

```text
input:
  file text
  RenderIR readonly
  WorkspaceIndex readonly

output:
  diagnostics
  snippets
  commands
  export transformers
```

Extensions sollen keine direkte UI-Manipulation bekommen. Sie liefern Daten an definierte Substrate.

## 9. Tests

Testebenen:

```text
core       RenderIR, Markdown, Layout, Export
io         atomisches Schreiben
ui_model   Commands, Input, Menüs
runtime    Datei-Workflow, Export, Clipboard
workspace  Index, Search, Symbols, Links, Lint, Snippets
```

## 10. Nächste tiefe Schritte

```text
v0.13.0 Markdown Extensions
v0.14.0 Export Pipeline: Reveal.js
v0.15.0 DOCX/PDF
v1.0.0 Extension ABI
```
