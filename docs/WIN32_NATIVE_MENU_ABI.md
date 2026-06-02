# Win32 Native Menu ABI v0.7

## Prinzip

Das Windows-Menü ist ein OS-Objekt, kein gezeichneter Text im Editorbereich.

```text
HMENU / WM_COMMAND
  -> CommandID
  -> EditorRuntime
  -> core/io
```

## Grenzen

`gui_win32/native_menu_win32.*` besitzt nur die Abbildung zwischen Win32-Menü-ID und `CommandID`.
Es speichert keinen Dokumentzustand und führt keine Datei- oder Buffer-Operationen aus.

## Status-Synchronisierung

Vor dem Öffnen eines Menüs (`WM_INITMENUPOPUP`) synchronisiert die Win32-Schicht den Enabled-State:

```text
Undo   <- runtime.can_undo()
Redo   <- runtime.can_redo()
Copy   <- selection active
Cut    <- selection active
Paste  <- runtime.clipboard() not empty
```

## Hotpath

Renderpfad und Menüpfad sind getrennt. Das Menü wird vom Betriebssystem gezeichnet; der Editor-Renderer zeichnet nur den Dokumentbereich.
