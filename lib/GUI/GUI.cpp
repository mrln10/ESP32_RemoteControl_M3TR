// lib/GUI/GUI.cpp
//
// GUI-State-Machine + Rendering (teilweise Updates, um Flackern zu minimieren)
//
// Bedienkonzept (zusammengefasst):
// - Normalmodus: kein Cursor sichtbar
// - Encoder Short-Press:
//     - wenn nicht im Edit: Edit aktivieren, Cursor sichtbar (start bei erster Stelle)
//     - wenn im Edit: Cursor weiter schieben (bei Frequenz 6 Stellen)
// - Encoder Drehen (nur im Edit):
//     - FRQ: ändere freq_hz gemäß Cursor-Stelle (mit Übertrag automatisch)
//     - MOD/PWR: zyklische Auswahl aus Liste
// - Encoder Long-Press:
//     - Edit beenden (Cursor weg)
//     - "Wert gespeichert" als Toast im Header (ersetzt Header-Text) für GUI_LIMITS.toast_ms
// - LEFT/RIGHT Buttons:
//     - Screenwechsel FRQ <-> MOD <-> PWR
//     - Edit wird dabei konservativ beendet (ohne Speichern)
//
// Rendering-Konzept (Anti-Flicker):
// - Das Display wird in 3 Zonen unterteilt:
//     1) Header (0..header_h-1)
//     2) Value Area (header_h..H-footer_h-1)
//     3) Footer (H-footer_h..H-1)
// - Statt Full-Clear wird nur die betroffene Zone gelöscht (fillRectRGB(..., schwarz))
//   und neu gezeichnet (Dirty Flags).

#include "GUI.h"

#include <gui_config.h>

#include <TFTDisplay.h>
#include <RotaryEncoder.h>
#include <NavButtons.h>

// --------------------
// Interner UI State
// --------------------
struct UIState {
  GuiScreen screen = GUI_FRQ;

  bool edit = false;           // true => Cursor sichtbar + Wert ist editierbar
  uint8_t cursor = 0;          // FRQ: 0..5 für "DDD.DDD"; Listen: 0

  // Solange millis() < toastUntil, wird im Header der Toast-Text angezeigt
  // (ersetzt die Überschrift)
  uint32_t toastUntil = 0;
};

static UIState ui;
static bool initialized = false;

// Werte / Auswahl-Indizes
static int32_t freq_hz = 0;    // Frequenz in Hz (Anzeige "DDD.DDD MHz")
static int modIndex = 0;       // Index in GUI_MOD_LIST
static int pwrIndex = 0;       // Index in GUI_PWR_LIST

// Dirty Flags für Teil-Redraws
static bool dirtyHeader = true;
static bool dirtyValue  = true;
static bool dirtyFooter = true;

// --------------------
// Utility Helpers
// --------------------

/**
 * @brief Modulo, das auch mit negativen Zahlen zuverlässig im Bereich [0..m-1] bleibt.
 */
static int modPos(int a, int m) {
  if (m <= 0) return 0;
  int r = a % m;
  return (r < 0) ? (r + m) : r;
}

/**
 * @brief Clamp (Begrenzen) eines int32-Werts auf [lo..hi].
 *        Empfohlen für Frequenzen, um niemals ungültige Werte zu erzeugen.
 */
static int32_t clampI32(int32_t v, int32_t lo, int32_t hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

/**
 * @brief Wrap eines int32-Werts in den Bereich [lo..hi] (zyklisch).
 *        Optional, falls frq_wrap = true in gui_config.h.
 */
static int32_t wrapI32(int32_t v, int32_t lo, int32_t hi) {
  if (lo > hi) return v;
  int32_t range = hi - lo + 1;
  int32_t x = v - lo;
  x %= range;
  if (x < 0) x += range;
  return lo + x;
}

/**
 * @brief Frequenz auf das kleinste Raster (1 kHz) zwingen und dann clamp/wrap anwenden.
 *        Dadurch ist die Anzeige stabil (keine "krummen" Schritte) und immer gültig.
 */
static void limitFreq() {
  // Auf das kleinste Raster (z.B. 1000 Hz = 1 kHz) zwingen
  int32_t step = GUI_LIMITS.frq_step_min_hz;
  if (step > 0) {
    int32_t rem = freq_hz % step;
    if (rem != 0) freq_hz -= rem;
  }

  // Grenzen anwenden
  if (GUI_LIMITS.frq_wrap) {
    freq_hz = wrapI32(freq_hz, GUI_LIMITS.frq_min_hz, GUI_LIMITS.frq_max_hz);
  } else {
    freq_hz = clampI32(freq_hz, GUI_LIMITS.frq_min_hz, GUI_LIMITS.frq_max_hz);
  }
}

/**
 * @brief Screenname für Header-Überschrift.
 */
static const char* screenName(GuiScreen s) {
  switch (s) {
    case GUI_FRQ: return "Frequenz";
    case GUI_MOD: return "Modulation";
    case GUI_PWR: return "Power";
    default: return "";
  }
}

// Default Font: 6x8 Pixel bei size=1
static int textW(const char* s, uint8_t size) { return (int)strlen(s) * 6 * size; }
static int textH(uint8_t size) { return 8 * size; }

/**
 * @brief Löscht einen Displaybereich, indem er schwarz gefüllt wird.
 *        (Damit vermeiden wir clearDisplay() und flackern weniger.)
 */
static void clearArea(int16_t x, int16_t y, int16_t w, int16_t h) {
  fillRectRGB(x, y, w, h, 0, 0, 0);
}

/**
 * @brief Zeichnet eine Linie in der Theme-Line-Farbe.
 */
static void lineTheme(int16_t x0, int16_t y0, int16_t x1, int16_t y1) {
  drawLineRGB(x0, y0, x1, y1,
              GUI_THEME.line_color.r, GUI_THEME.line_color.g, GUI_THEME.line_color.b);
}

/**
 * @brief Zeichnet eine Linie in der Cursor-Farbe.
 */
static void lineCursor(int16_t x0, int16_t y0, int16_t x1, int16_t y1) {
  drawLineRGB(x0, y0, x1, y1,
              GUI_THEME.cursor_color.r, GUI_THEME.cursor_color.g, GUI_THEME.cursor_color.b);
}

/**
 * @brief Bestimmt die Schrittweite (Hz) für die aktuelle Cursor-Position bei "DDD.DDD MHz".
 *
 * Cursor-Stellen (0..5) entsprechen:
 * 0: 100 MHz  -> 100,000,000 Hz
 * 1: 10  MHz  -> 10,000,000 Hz
 * 2: 1   MHz  -> 1,000,000 Hz
 * 3: 100 kHz  -> 100,000 Hz
 * 4: 10  kHz  -> 10,000 Hz
 * 5: 1   kHz  -> 1,000 Hz
 */
static int32_t cursorStepHz(uint8_t cursor) {
  static const int32_t steps[6] = {
    100000000, 10000000, 1000000, 100000, 10000, 1000
  };
  if (cursor > 5) cursor = 5;
  return steps[cursor];
}

/**
 * @brief Formatiert freq_hz als "DDD.DDD" (MHz) ohne float.
 *
 * Beispiel:
 *   104'200'000 Hz
 *   -> 104'200 kHz (freq_hz/1000)
 *   -> mhz_int=104, frac=200
 *   -> "104.200"
 */
static void formatFreq(char out[8]) {
  int32_t kHz = freq_hz / 1000;
  int32_t mhz_int = kHz / 1000; // 30..511
  int32_t frac = kHz % 1000;    // 0..999

  out[0] = char('0' + (mhz_int / 100) % 10);
  out[1] = char('0' + (mhz_int / 10) % 10);
  out[2] = char('0' + (mhz_int / 1) % 10);
  out[3] = '.';
  out[4] = char('0' + (frac / 100) % 10);
  out[5] = char('0' + (frac / 10) % 10);
  out[6] = char('0' + (frac / 1) % 10);
  out[7] = '\0';
}

// --------------------
// Rendering: Teilbereiche
// --------------------

/**
 * @brief Rendert den Headerbereich.
 *
 * Verhalten:
 * - Wenn Toast aktiv: Header zeigt "Gespeichert" (zentriert) an
 * - Sonst: Header zeigt Überschrift (aktueller Screen) links an
 * - Trennlinie am unteren Rand des Headers
 */
static void renderHeaderArea(int16_t W) {
  clearArea(0, 0, W, GUI_LIMITS.header_h);

  const bool toastActive = (millis() < ui.toastUntil);

  if (toastActive) {
    const char* msg = "Gespeichert";
    uint8_t size = 2;
    int w = textW(msg, size);
    int x = (W - w) / 2;
    if (x < 6) x = 6;

    drawText(msg, x, 6, size,
             GUI_THEME.toast_color.r, GUI_THEME.toast_color.g, GUI_THEME.toast_color.b);
  } else {
    drawText(screenName(ui.screen), 6, 6, GUI_THEME.header_size,
             GUI_THEME.header_text.r, GUI_THEME.header_text.g, GUI_THEME.header_text.b);
  }

  // Trennlinie am unteren Rand des Headers
  lineTheme(0, GUI_LIMITS.header_h - 1, W - 1, GUI_LIMITS.header_h - 1);
}

/**
 * @brief Rendert den Footerbereich (Menüleiste unten).
 * - Trennlinie oben
 * - FRQ/MOD/PWR Labels, aktives Label wird farblich hervorgehoben
 * - Rechts "ON" als Platzhalter (später echtes Symbol)
 */
static void renderFooterArea(int16_t W, int16_t H) {
  int y0 = H - GUI_LIMITS.footer_h;
  clearArea(0, y0, W, GUI_LIMITS.footer_h);

  lineTheme(0, y0, W - 1, y0);

  auto col = [&](GuiScreen s, uint8_t &r, uint8_t &g, uint8_t &b) {
    if (ui.screen == s) { r = GUI_THEME.footer_active.r; g = GUI_THEME.footer_active.g; b = GUI_THEME.footer_active.b; }
    else { r = GUI_THEME.footer_idle.r; g = GUI_THEME.footer_idle.g; b = GUI_THEME.footer_idle.b; }
  };

  int footerTextY = y0 + 6;

  uint8_t r,g,b;
  col(GUI_FRQ, r,g,b); drawText("FRQ", 10, footerTextY, GUI_THEME.footer_size, r,g,b);
  col(GUI_MOD, r,g,b); drawText("MOD", 50, footerTextY, GUI_THEME.footer_size, r,g,b);
  col(GUI_PWR, r,g,b); drawText("PWR", 90, footerTextY, GUI_THEME.footer_size, r,g,b);

  drawText("ON", W - 28, footerTextY, GUI_THEME.footer_size, 0, 255, 0);
}

/**
 * @brief Rendert die Frequenzanzeige in der Value Area:
 * - "DDD.DDD" groß
 * - "MHz" kleiner als Einheit
 * - Cursor als Unterstrich unter der aktiven Stelle (nur im Edit)
 */
static void renderFRQ(int16_t W, int16_t H) {
  const uint8_t valueSize = GUI_THEME.value_size;
  const uint8_t unitSize  = GUI_THEME.unit_size;
  const int gapPx = 2 * valueSize;

  char frqStr[8];
  formatFreq(frqStr);

  const char* unit = "MHz";

  const int valueWidth = textW(frqStr, valueSize);
  const int unitWidth  = textW(unit, unitSize);
  const int totalWidth = valueWidth + gapPx + unitWidth;

  // Zentrierung bezogen auf das Gesamt-Display (Value Area ist vorher gelöscht)
  const int startX = (W - totalWidth) / 2;
  const int y = (H / 2) - (textH(valueSize) / 2);

  drawText(frqStr, startX, y, valueSize,
           GUI_THEME.value_text.r, GUI_THEME.value_text.g, GUI_THEME.value_text.b);

  // Einheit optisch an die Basislinie anpassen
  const int unitX = startX + valueWidth + gapPx;
  const int unitY = y + textH(valueSize) - textH(unitSize);
  drawText(unit, unitX, unitY, unitSize,
           GUI_THEME.unit_text.r, GUI_THEME.unit_text.g, GUI_THEME.unit_text.b);

  // Cursor (Unterstrich) nur im Edit
  if (ui.edit) {
    // Cursor 0..5 mappt auf Zeichenindex in "DDD.DDD" (Punkt ist an Index 3)
    const int charIndex = (ui.cursor <= 2) ? ui.cursor : (ui.cursor + 1);

    const int charWpx = 6 * valueSize;
    const int charHpx = 8 * valueSize;

    const int underlineX0 = startX + charIndex * charWpx;
    const int underlineX1 = underlineX0 + charWpx - 2;
    const int underlineY  = y + charHpx + valueSize;

    lineCursor(underlineX0, underlineY, underlineX1, underlineY);
  }
}

/**
 * @brief Rendert einen Listenwert (z.B. Modulation/Power) mittig.
 *        Optional wird ein Cursor als Unterstrich unter dem gesamten Text gezeichnet.
 */
static void renderListValue(int16_t W, int16_t H, const char* value) {
  const uint8_t size = GUI_THEME.value_size + 1; // etwas größer für kurze Strings

  const int w = textW(value, size);
  const int h = textH(size);

  const int x = (W - w) / 2;
  const int y = (H / 2) - (h / 2);

  drawText(value, x, y, size,
           GUI_THEME.value_text.r, GUI_THEME.value_text.g, GUI_THEME.value_text.b);

  if (ui.edit) {
    const int underlineY = y + h + size;
    lineCursor(x, underlineY, x + w - 2, underlineY);
  }
}

/**
 * @brief Rendert die komplette Value Area (mittlerer Bereich).
 *        Wird bei Wertänderungen/Cursoränderungen neu gezeichnet.
 */
static void renderValueArea(int16_t W, int16_t H) {
  const int y0 = GUI_LIMITS.header_h;
  const int h  = H - GUI_LIMITS.header_h - GUI_LIMITS.footer_h;

  // Nur den zentralen Bereich löschen, nicht das ganze Display
  clearArea(0, y0, W, h);

  switch (ui.screen) {
    case GUI_FRQ:
      renderFRQ(W, H);
      break;
    case GUI_MOD:
      renderListValue(W, H, (GUI_MOD_COUNT > 0) ? GUI_MOD_LIST[modIndex] : "---");
      break;
    case GUI_PWR:
      renderListValue(W, H, (GUI_PWR_COUNT > 0) ? GUI_PWR_LIST[pwrIndex] : "---");
      break;
  }
}

/**
 * @brief Rendert nur die als "dirty" markierten Zonen.
 *        Dadurch minimieren wir Flackern und unnötige Arbeit.
 */
static void renderDirty() {
  int16_t W, H;
  getDisplaySize(W, H);

  if (dirtyHeader) { renderHeaderArea(W); dirtyHeader = false; }
  if (dirtyValue)  { renderValueArea(W, H); dirtyValue = false; }
  if (dirtyFooter) { renderFooterArea(W, H); dirtyFooter = false; }
}

// --------------------
// Actions (State Transitions)
// --------------------

/**
 * @brief Aktiviert Edit-Mode (Cursor sichtbar, Drehen ändert Werte).
 */
static void enterEdit() {
  ui.edit = true;
  ui.cursor = 0;
}

/**
 * @brief Beendet Edit-Mode, zeigt Toast im Header (ersetzt Überschrift).
 */
static void exitEditAndSave() {
  ui.edit = false;
  ui.cursor = 0;
  ui.toastUntil = millis() + GUI_LIMITS.toast_ms;
}

/**
 * @brief Cursor weiterschieben:
 * - FRQ: 6 Stellen (0..5)
 * - Listen: Cursor bleibt 0
 */
static void nextCursorPosition() {
  if (ui.screen == GUI_FRQ) ui.cursor = (ui.cursor + 1) % 6;
  else ui.cursor = 0;
}

/**
 * @brief Wertänderung in Abhängigkeit vom Screen:
 * - FRQ: freq_hz += delta * cursorStepHz(cursor)
 * - MOD/PWR: zyklisches Durchschalten der Listen
 */
static void changeValueByDelta(int32_t d) {
  if (d == 0) return;

  if (ui.screen == GUI_FRQ) {
    freq_hz += (int32_t)d * cursorStepHz(ui.cursor);
    limitFreq();
  } else if (ui.screen == GUI_MOD) {
    if (GUI_MOD_COUNT > 0) modIndex = modPos(modIndex + (int)d, GUI_MOD_COUNT);
  } else if (ui.screen == GUI_PWR) {
    if (GUI_PWR_COUNT > 0) pwrIndex = modPos(pwrIndex + (int)d, GUI_PWR_COUNT);
  }
}

/**
 * @brief Screenwechsel per LEFT/RIGHT.
 *        Konservatives UX: Edit wird beendet (ohne Speichern).
 */
static void switchScreenByDelta(int delta) {
  ui.edit = false;
  ui.cursor = 0;

  int s = (int)ui.screen + delta;
  s = modPos(s, 3);
  ui.screen = (GuiScreen)s;
}

// --------------------
// Public API
// --------------------

/**
 * @brief Initialisiert GUI-Status und setzt Defaults aus gui_config.
 *        Danach wird einmal initial gerendert (Dirty Flags => Teil-Render).
 */
void guiInit() {
  ui = UIState{};
  ui.screen = GUI_FRQ;

  freq_hz  = GUI_DEFAULTS.frq_start_hz;
  modIndex = modPos(GUI_DEFAULTS.mod_index, GUI_MOD_COUNT);
  pwrIndex = modPos(GUI_DEFAULTS.pwr_index, GUI_PWR_COUNT);

  // Frequenz in Grenzen + Raster bringen
  limitFreq();

  initialized = true;

  // Einmal Full-Clear für sauberen Start, danach nur noch Teil-Redraws
  clearDisplay();

  dirtyHeader = dirtyValue = dirtyFooter = true;
  renderDirty();
}

/**
 * @brief Hauptupdate der GUI:
 * - Liest Eingaben (Encoder + Buttons)
 * - Aktualisiert State Machine
 * - Markiert betroffene Bereiche "dirty"
 * - Rendert am Ende nur die dirty Zonen
 */
void guiUpdate() {
  if (!initialized) return;

  updateRotaryEncoder();
  updateNavButtons();

  // --- LEFT/RIGHT: Screenwechsel ---
  if (getLeftPressed()) {
    switchScreenByDelta(-1);
    dirtyHeader = dirtyValue = dirtyFooter = true;
  }
  if (getRightPressed()) {
    switchScreenByDelta(+1);
    dirtyHeader = dirtyValue = dirtyFooter = true;
  }

  // --- Encoder Long-Press: speichern + exit edit + toast ---
  if (getButtonLongPressed()) {
    exitEditAndSave();

    // Header zeigt Toast statt Titel
    dirtyHeader = true;

    // Cursor verschwindet (Value Area muss neu)
    dirtyValue = true;
  }

  // --- Encoder Short-Press: edit togglen / cursor weiterschieben ---
  if (getButtonPressed()) {
    if (!ui.edit) enterEdit();
    else nextCursorPosition();

    // Cursor/Mode liegt in Value Area
    dirtyValue = true;
  }

  // --- Encoder drehen: nur im Edit Mode ---
  int32_t d = getEncoderDelta();
  if (d != 0 && ui.edit) {
    changeValueByDelta(d);

    // Wert hat sich geändert => Value Area neu
    dirtyValue = true;
  }

  // --- Toast abgelaufen? => Header wieder normal zeichnen ---
  static bool toastWasActive = false;
  const bool toastActive = (millis() < ui.toastUntil);
  if (toastWasActive && !toastActive) {
    dirtyHeader = true;
  }
  toastWasActive = toastActive;

  // --- Render wenn nötig ---
  if (dirtyHeader || dirtyValue || dirtyFooter) {
    renderDirty();
  }
}

/**
 * @brief Erzwingt ein Redraw aller Bereiche (z.B. nach Rotation/Layout-Änderungen).
 */
void guiForceRedraw() {
  if (!initialized) return;
  dirtyHeader = dirtyValue = dirtyFooter = true;
  renderDirty();
}

/**
 * @brief Liefert aktuellen Screen zurück (optional für Debug/Integration).
 */
GuiScreen guiGetScreen() { return ui.screen; }

/**
 * @brief true wenn im Edit-Mode (Cursor sichtbar, Drehen ändert Werte).
 */
bool guiIsEditing() { return ui.edit; }
