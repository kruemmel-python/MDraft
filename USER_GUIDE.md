# MDraft User Guide

**Product:** MDraft — Native Markdown Workspace Editor  
**Developer:** Ralf Krümmel

Version: **v0.12.3**

## 1. Datei-Workflow

```text
Datei
  Neu
  Öffnen...
  Speichern
  Speichern unter...
  Als HTML exportieren
    Standard
    Cyberpunk-Stil
    Dystopie-Stil
    Horror-Stil
    Abenteuer-/Spannung-Stil
  Beenden
```

Wenn ein Dokument noch keinen Pfad besitzt und du **Speichern** drückst, öffnet MDraft automatisch **Speichern unter...**.

## 2. Preview

```text
Ansicht
  RenderIR/HTML Live-Vorschau anzeigen    Ctrl+P
  Preview sperren                         Ctrl+Shift+P
  Preview-Thema
    Standard
    Cyberpunk
    Dystopie
    Horror
    Abenteuer
```

### Live-Modus

Preview folgt dem aktuellen Editor-Buffer.

### Locked-Modus

Preview friert den aktuellen Markdown-Snapshot ein. Du kannst danach andere Dateien öffnen oder weiter schreiben, ohne dass die Preview wechselt.

## 3. Workspace

```text
Projekt
  Workspace öffnen...
  Workspace neu indizieren
  Workspace suchen...          Ctrl+Shift+F
  Symbol-Navigation...         Ctrl+Shift+O
  Links validieren...          Ctrl+Shift+L
  Bildpfad-Vorschläge...       Ctrl+Shift+I
  Diagnostics/Linting...       Ctrl+Shift+D
  Git-Status light...          Ctrl+Shift+G
  Workspace-Status
```

Ein Workspace ist ein Ordner, in dem MDraft Markdown-Dateien, Bilder und Links indiziert.

## 4. Multi-Datei-Suche

- Wenn Text markiert ist, wird die Markierung als Suchbegriff genutzt.
- Sonst wird das Wort unter dem Cursor verwendet.
- Ergebnisse enthalten Datei, Zeile, Spalte und Snippet.

## 5. Symbol-Navigation

Symbole entstehen aus Markdown-Headings:

```markdown
# Kapitel
## Abschnitt
```

Die Navigation kann eine Query verwenden oder alle Symbole anzeigen.

## 6. Link-Validierung

MDraft prüft:

```markdown
[Text](relative/datei.md)
![Alt](images/bild.png)
```

Kaputte Links erscheinen als Diagnostics.

## 7. Bildpfad-Vorschläge

MDraft scannt Workspace-Assets und schlägt Bildpfade vor für:

```text
png
jpg
jpeg
gif
webp
bmp
svg
```

## 8. Snippets

```text
Bearbeiten
  Snippet einfügen       Ctrl+J
  Bild einfügen...
```

Trigger schreiben und `Ctrl+J` drücken:

```text
h1       # ...
h2       ## ...
todo     - [ ] ...
table    Markdown-Tabelle
math     $$ ... $$
mermaid  Mermaid Flowchart
code     Code Fence
img      ![...](...)
note     > **Notiz:** ...
```

## 9. Bilder

Markdown:

```markdown
![Logo](images/logo.png)
```

Datenfluss:

```text
Markdown Image
  -> RenderIR DrawKind::Image
  -> Win32 Preview via GDI+
  -> SVG/HTML Export via <image href="...">
```

## 10. Diagnostics/Linting

Aktuelle Regeln:

```text
MD001 trailing whitespace
MD002 tabs
MD003 long line
MD004 heading level jump
MD005 duplicate heading anchor
MD006 image without alt text
MD007 link without visible text
MD008 malformed pipe table
MD009 missing H1
MD010 multiple H1
```

## 11. Git-Status light

Voraussetzung: `git.exe` im PATH.

MDraft ruft intern auf:

```text
git status --porcelain=v1
```

Erkannt werden:

```text
modified
added
deleted
renamed
untracked
conflict
```

## 12. CLI

```powershell
mdraft-cli file.md --theme horror
```

Themes:

```text
standard
cyberpunk
dystopie
horror
abenteuer
```
