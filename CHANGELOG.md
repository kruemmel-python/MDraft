# Changelog

## v0.12.6 - GitHub-style Markdown preview

- Added a GitHub preview/export theme for README-style Markdown rendering.
- Uses GitHub-like page width, white document surface, heading scale, borders and muted text colors.
- Renders GitHub-mode prose/headings with a sans-serif UI font while keeping code blocks monospace.
- Adds Win32 menu entries for GitHub preview and GitHub HTML export.

## v0.12.5 - Win32 UTF-8 editor rendering

- Fixed Win32 editor rendering for German umlauts and other UTF-8 characters.
- Draws complete UTF-8 codepoints instead of isolated bytes in the native editor pane.
- Aligns glyph measurement and mouse/cursor mapping with visible UTF-8 characters.
- Keeps the RenderIR/HTML preview and exported HTML UTF-8 behavior unchanged.

## v0.12.4 - Preview layout and theme readability

- Wrapped long RenderIR headings to the available preview width.
- Rebalanced heading, code and quote colors across Standard, Cyberpunk, Dystopia, Horror and Adventure themes.
- Kept numbered Markdown list items visually separated in the RenderIR preview instead of merging them into one paragraph.
- Truncated the Win32 preview metadata line so it no longer overlaps the theme status badge.
- Moved Win32/X11/About version strings to the CMake-provided `MDRAFT_VERSION` definition.

## v0.12.3 - Enterprise repository consolidation

- Consolidated the public product identity from MarkTeX Native to MDraft.
- Standardized CMake targets, build scripts and package metadata on `mdraft-*`.
- Added enterprise repository documentation: architecture, user guide and feature matrix.
- Added per-machine MSI packaging metadata and Start Menu registration for Windows.
- Preserved the native C++ substrate: core, io, ui_model, runtime, workspace and Win32 GUI.

## v0.12.1 - GDI+ MSVC include substrate fix

- Fixed MSVC/GDI+ compilation with `WIN32_LEAN_AND_MEAN`.
- Kept the RenderIR image path enabled for Win32 preview and SVG/HTML export.

## v0.12.0 - Snippets and image workflow

- Added workspace-aware snippets.
- Added image insertion and image path suggestion workflows.

## v0.10.x - Workspace substrate

- Added workspace indexing, multi-file search, symbol navigation, link validation and diagnostics.

## v0.9.x - RenderIR workflow

- Added RenderIR, live preview, themed export and preview workflow fixes.
