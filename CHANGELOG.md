# Changelog

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
