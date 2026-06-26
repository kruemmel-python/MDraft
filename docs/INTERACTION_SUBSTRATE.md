# MarkTeX Native v0.5 Interaction Substrate

## Prinzip

Die UI stellt den Text nicht "nur dar". Sie ist eine Sicht auf ein modelliertes Editor-Substrat.

```text
Event-Pfad:
  X11 Event
    -> InputDispatcher / MenuBar / SelectionEngine
    -> CommandID oder EditorState-Mutation
    -> CommandDispatcher
    -> CommandManager
    -> GapBuffer

Render-Pfad:
  Invalidation
    -> Renderer liest EditorState + GapBuffer
    -> HighlightProcessor baut AttributeBuffer
    -> CursorManager berechnet Byte -> Pixel
    -> X11 projiziert Glyphen und Affordanzen
```

## Neue ABI-Module

### CommandRegistry

Zentrales Register für Befehle.

```cpp
CommandID Save;
CommandID ExportHtml;
CommandID Undo;
CommandID Redo;
CommandID SelectAll;
```

Menü und Tastatur fragen beide dieselbe Registry. Dadurch ist `Ctrl+S` semantisch identisch mit `File:Save`.

### MenuBar

Reine Koordinaten-/Befehlsstruktur. Keine Datei- oder Bufferlogik.

```cpp
CommandID command_at(int x, int y) const;
```

### SelectionEngine

Hält Drag-Interaktion aus dem Renderer heraus.

```cpp
begin_drag(EditorState&, byte);
update_drag(EditorState&, byte);
end_drag(EditorState&);
select_all(EditorState&, size);
```

### CursorManager

Berechnet die sichtbare Cursorprojektion aus Byteadresse, Viewport und Glyph-Metriken.

```cpp
CursorVisual visual_for_byte(...);
```

### ContextProjection

Minimaler Kontextkanal für Rechtsklick. Er identifiziert den Token-Typ unter einer Byteadresse über den `AttributeBuffer`.

## Warum das notwendig ist

Ein Editor mit "Notepad-Gefühl" darf nicht warten, bis Parsing, Export oder UI-Dekoration abgeschlossen sind. Deshalb sind Event-Pfad und Render-Pfad getrennt:

- Input mutiert Zustand und setzt Invalidation.
- Render liest Zustand und zeichnet.
- Parser/Highlighting ist eine Projektion, nicht der Eigentümer des Textes.

## Grenzen

Das Clipboard ist in v0.5 bewusst intern. X11-Selection/CLIPBOARD wird erst in einem späteren ABI-Schritt angebunden, damit Copy/Cut/Paste als Editor-Transaktionen zuerst korrekt sind.
