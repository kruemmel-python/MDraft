# MDraft Feature Matrix

**Product:** MDraft — Native Markdown Workspace Editor  
**Developer:** Ralf Krümmel

Version: **v0.12.6**

| Bereich | Status | Substrat | Hinweise |
|---|---:|---|---|
| Nativer Editor | Stabiler Kern | runtime + ui_model + gui_win32 | Win32-UI, GapBuffer, Auswahl, Cursor |
| Datei öffnen | Aktiv | runtime + gui_win32 | GetOpenFileNameW |
| Speichern / Speichern unter | Aktiv | runtime + io + gui_win32 | atomisches Schreiben, SaveAs-Fallback |
| Theme-Export | Aktiv | core/html + RenderIR | Standard, GitHub, Cyberpunk, Dystopie, Horror, Abenteuer |
| Live Preview | Aktiv | RenderIR + GDI | GitHub-README-Modus plus Theme-Preview |
| Preview-Locking | Aktiv | gui_win32 PreviewState | Snapshot-Preview |
| Preview-Scroll | Aktiv | DisplayList height | eigener Scrollzustand |
| RenderIR | Aktiv | core/render_ir | Rect, Line, Text, Image |
| SVG/HTML Export | Aktiv | core/render_svg | DisplayList -> SVG-in-HTML |
| Markdown Tabellen | Aktiv | core/layout_engine | Pipe-Table State Machine |
| Math-Subset | Aktiv | core/layout_engine | Greek, Fraction, Sup/Sub, einfache Formen |
| Mermaid-Subset | Aktiv | core/layout_engine | Flowchart-Subset |
| Bilder in Preview | Aktiv | RenderIR + GDI+ | GDI+ Bildanzeige |
| Bilder im Export | Aktiv | RenderIR + SVG | `<image href="...">` |
| Workspace öffnen | Aktiv | workspace | WorkspaceRoot |
| FileIndex | Aktiv | workspace/file_index | Markdown-Dateien + Assets |
| Multi-Datei-Suche | Aktiv | workspace_search | Datei/Zeile/Spalte/Snippet |
| Symbol-Navigation | Aktiv | markdown_symbols + workspace_symbol_nav | Headings/Anchors |
| Link-Validierung | Aktiv | link_resolver + link_validation | relative Links |
| Bildpfad-Vorschläge | Aktiv | image_suggestions | Workspace Assets |
| Diagnostics/Linting | Aktiv | workspace_lint + diagnostics | MD001-MD010 |
| Git-Status light | Aktiv | git_adapter | `git.exe` Prozessadapter |
| Snippets | Aktiv | snippets | Trigger + Cursor-Platzhalter |
| MSI Packaging | Aktiv | CPack/WiX | Startmenü-Shortcut |
| Spellcheck | Geplant | Diagnostics | v0.13+ oder später |
| Markdown Extensions | Geplant | Extension-ready Parser | v0.13.0 |
| Reveal.js Export | Geplant | Export Pipeline | v0.14.0 |
| DOCX/PDF Export | Geplant | Export Pipeline | v0.15.0 |
| Extension ABI | Geplant | Workspace + RenderIR readonly | v1.0.0 |
