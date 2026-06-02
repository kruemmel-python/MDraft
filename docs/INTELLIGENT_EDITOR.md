# MDraft v0.4 — Intelligent-Editor-Substrat

## Prinzip

Ein Editor ist keine Textanzeige. Er ist eine Zustandsmaschine über einem Textspeicher.
Deshalb darf ein Tastendruck nicht direkt `GapBuffer::insert()` auslösen.

Pfad:

```text
OS Event
  -> InputDispatcher
  -> EditorAction
  -> CommandDispatcher
  -> CommandManager
  -> GapBuffer
  -> HighlightProcessor
  -> Renderer
```

## Core ABI: CommandManager

`CommandManager` kapselt jede Mutation als `EditCommand`.

```cpp
struct EditCommand {
  EditKind kind;
  std::size_t pos;
  std::string inserted;
  std::string erased;
};
```

Damit existiert die inverse Operation physisch im RAM. Undo/Redo ist kein nachträglicher Hack,
sondern Teil des Mutationssubstrats.

## Smart Newline

`CommandManager::smart_newline()` analysiert die Zeile vor dem Cursor:

```text
- item       -> \n- 
- [x] done   -> \n- [ ] 
9. nine      -> \n10. 
```

Die Logik liegt im Core, weil sie Dokumentsemantik ist. Die UI triggert nur `ActionKind::SmartNewline`.

## UI ABI: InputDispatcher

Der InputDispatcher enthält keine X11-Objekte. X11 normalisiert KeyPress zu:

```cpp
struct InputEvent {
  InputKey key;
  std::string text;
  bool ctrl;
  bool shift;
};
```

Dann wird daraus eine EditorAction. Dadurch kann später Wayland, SDL2 oder ein eigener FB-Backend
dieselbe Editor-Action-ABI verwenden.

## UI ABI: GlyphMetricsTable

Mauskoordinaten dürfen nicht mit `col = x / 8` verheiratet sein.
`GlyphMetricsTable` ist die physische Mapping-Schicht:

```text
byte range -> pixel advance
pixel x    -> nearest byte offset
```

Aktuell nutzt der X11-Pfad eine monospaced Core-Font-Metrik. Die ABI ist aber für variable
Glyphbreiten oder einen FreeType-/Bitmapfont-Atlas vorbereitet.
