# MDraft Changelog

**Product:** MDraft — Native Markdown Workspace Editor  
**Developer:** Ralf Krümmel

## v0.12.2 - Product Identity / Documentation Consolidation

Codename: **Workspace Snippets Images**  
Status: **Developer Preview / Stable Core**

### Added

- Professional README
- `ARCHITECTURE.md`
- `USER_GUIDE.md`
- `FEATURE_MATRIX.md`
- Consolidated `CHANGELOG.md`
- Packaging installs documentation files into MSI target directory

### Notes

This release consolidates the product identity after the RenderIR and Workspace evolution. It does not add runtime behavior; it stabilizes the documentation boundary and describes the architecture as a coherent native Markdown Workspace Editor.

## v0.12.1 - Snippets + Images MSVC Fix

- Fixed GDI+ include order under MSVC and `WIN32_LEAN_AND_MEAN`
- Added explicit COM/GUID includes before `gdiplus.h`
- Kept RenderIR image path active

## v0.12.0 - Snippets + Images

- Added snippets subsystem
- Added Markdown image insertion
- Added RenderIR `DrawKind::Image`
- Added GDI+ image preview
- Added SVG `<image>` export

## v0.11.2 - Git-Status light

- Added `git_adapter`
- Added `git status --porcelain=v1` parsing
- Added Git status UI command

## v0.11.1 - MSVC Lifetime Fix

- Fixed returning reference to temporary preview markdown source
- Preview source now returns by value

## v0.11.0 - Diagnostics / Linting Substrate

- Added workspace linting
- Extended Diagnostic model
- Added Markdown lint rules MD001-MD010

## v0.10.4 - Preview Locking

- Added Preview locked/unlocked state
- Locked preview snapshots current Markdown
- Preview remains stable across file changes

## v0.10.3 - Link Validation + Image Suggestions

- Added link validation
- Added image path suggestions
- Added asset indexing

## v0.10.2 - Symbol Navigation

- Added workspace symbol navigation
- Added heading search/open flow

## v0.10.1 - Multi-Datei-Suche

- Added workspace search
- Added file/line/column/snippet search results

## v0.10.0 - Workspace + Index Substrate

- Added workspace module
- Added FileIndex
- Added MarkdownSymbols
- Added LinkResolver
- Added Diagnostics foundation

## v0.9.x - RenderIR Substrate

- Introduced DisplayList / RenderIR
- Unified preview and export around the same render source
- Added Preview scroll and themes

## v0.8.x - Native HTML Themes and File Workflow

- Added Cyberpunk, Dystopie, Horror, Abenteuer themes
- Added Save As, New, Open file workflow
- Added native file dialogs
- Added Windows save error handling

## v0.7.x and earlier

- Native editor substrate
- Markdown parsing
- HTML export
- Win32 menu and UI model
