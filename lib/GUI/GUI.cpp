#include "GUI.h"

#include <gui_config.h>
#include <TFTDisplay.h>
#include <RotaryEncoder.h>
#include <NavButtons.h>

struct UIState {
  GuiScreen screen = GUI_FRQ;

  // Edit Mode (Cursor sichtbar) gilt nur für FRQ/MOD/PWR
  bool edit = false;
  uint8_t cursor = 0;              // FRQ: 0..5, Listen: 0

  // Footer Fokus
  GuiFooterItem focus = FOOT_FRQ;

  // Toast im Header
  uint32_t toastUntil = 0;

  // Radio Status (nur Anzeige)
  bool radioOn = false;

  // Events für main
  bool onToggleRequested = false;
  bool saveRequested = false;
  GuiScreen savedScreen = GUI_FRQ;
};

static UIState ui;
static bool initialized = false;

static int32_t freq_hz = 0;
static int modIndex = 0;
static int pwrIndex = 0;

static bool dirtyHeader = true;
static bool dirtyValue  = true;
static bool dirtyFooter = true;

// ---------- Helpers ----------
static int modPos(int a, int m) {
  if (m <= 0) return 0;
  int r = a % m;
  return (r < 0) ? (r + m) : r;
}

static int32_t clampI32(int32_t v, int32_t lo, int32_t hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

static int32_t wrapI32(int32_t v, int32_t lo, int32_t hi) {
  if (lo > hi) return v;
  int32_t range = hi - lo + 1;
  int32_t x = v - lo;
  x %= range;
  if (x < 0) x += range;
  return lo + x;
}

static void limitFreq() {
  int32_t step = GUI_LIMITS.frq_step_min_hz;
  if (step > 0) {
    int32_t rem = freq_hz % step;
    if (rem != 0) freq_hz -= rem;
  }
  if (GUI_LIMITS.frq_wrap) freq_hz = wrapI32(freq_hz, GUI_LIMITS.frq_min_hz, GUI_LIMITS.frq_max_hz);
  else                     freq_hz = clampI32(freq_hz, GUI_LIMITS.frq_min_hz, GUI_LIMITS.frq_max_hz);
}

static const char* screenName(GuiScreen s) {
  switch (s) {
    case GUI_FRQ: return "Frequenz";
    case GUI_MOD: return "Modulation";
    case GUI_PWR: return "Power";
    default: return "";
  }
}

static int textW(const char* s, uint8_t size) { return (int)strlen(s) * 6 * size; }
static int textH(uint8_t size) { return 8 * size; }

static void clearArea(int16_t x, int16_t y, int16_t w, int16_t h) {
  fillRectRGB(x, y, w, h, 0, 0, 0);
}

static void lineTheme(int16_t x0, int16_t y0, int16_t x1, int16_t y1) {
  drawLineRGB(x0, y0, x1, y1, GUI_THEME.line_color.r, GUI_THEME.line_color.g, GUI_THEME.line_color.b);
}

static void lineCursor(int16_t x0, int16_t y0, int16_t x1, int16_t y1) {
  drawLineRGB(x0, y0, x1, y1, GUI_THEME.cursor_color.r, GUI_THEME.cursor_color.g, GUI_THEME.cursor_color.b);
}

static int32_t cursorStepHz(uint8_t cursor) {
  static const int32_t steps[6] = { 100000000, 10000000, 1000000, 100000, 10000, 1000 };
  if (cursor > 5) cursor = 5;
  return steps[cursor];
}

// freq_hz -> "DDD.DDD" MHz (ohne float)
static void formatFreq(char out[8]) {
  int32_t kHz = freq_hz / 1000;
  int32_t mhz_int = kHz / 1000;
  int32_t frac = kHz % 1000;

  out[0] = char('0' + (mhz_int / 100) % 10);
  out[1] = char('0' + (mhz_int / 10) % 10);
  out[2] = char('0' + (mhz_int / 1) % 10);
  out[3] = '.';
  out[4] = char('0' + (frac / 100) % 10);
  out[5] = char('0' + (frac / 10) % 10);
  out[6] = char('0' + (frac / 1) % 10);
  out[7] = '\0';
}

// ---------- Rendering ----------
static void renderHeaderArea(int16_t W) {
  clearArea(0, 0, W, GUI_LIMITS.header_h);

  bool toastActive = (millis() < ui.toastUntil);
  if (toastActive) {
    const char* msg = "Wert gespeichert";
    uint8_t size = 2;
    int w = textW(msg, size);
    int x = (W - w) / 2;
    if (x < 6) x = 6;
    drawText(msg, x, 6, size, GUI_THEME.toast_color.r, GUI_THEME.toast_color.g, GUI_THEME.toast_color.b);
  } else {
    drawText(screenName(ui.screen), 6, 6, GUI_THEME.header_size,
             GUI_THEME.header_text.r, GUI_THEME.header_text.g, GUI_THEME.header_text.b);
  }

  lineTheme(0, GUI_LIMITS.header_h - 1, W - 1, GUI_LIMITS.header_h - 1);
}

static void drawFooterItem(const char* label, int x, int y, bool focused) {
  uint8_t r, g, b;
  if (focused) { r = GUI_THEME.footer_active.r; g = GUI_THEME.footer_active.g; b = GUI_THEME.footer_active.b; }
  else         { r = GUI_THEME.footer_idle.r;   g = GUI_THEME.footer_idle.g;   b = GUI_THEME.footer_idle.b; }
  drawText(label, x, y, GUI_THEME.footer_size, r, g, b);
}

static void renderFooterArea(int16_t W, int16_t H) {
  int y0 = H - GUI_LIMITS.footer_h;
  clearArea(0, y0, W, GUI_LIMITS.footer_h);
  lineTheme(0, y0, W - 1, y0);

  int y = y0 + 6;

  drawFooterItem("FRQ", 10, y, ui.focus == FOOT_FRQ);
  drawFooterItem("MOD", 50, y, ui.focus == FOOT_MOD);
  drawFooterItem("PWR", 90, y, ui.focus == FOOT_PWR);

  // ON Bereich rechts: Fokus hebt "ON"/"OFF" hervor
  if (ui.radioOn) {
    if (ui.focus == FOOT_ON) drawText("ON",  W - 28, y, GUI_THEME.footer_size, GUI_THEME.footer_active.r, GUI_THEME.footer_active.g, GUI_THEME.footer_active.b);
    else                     drawText("ON",  W - 28, y, GUI_THEME.footer_size, 0, 255, 0);
  } else {
    if (ui.focus == FOOT_ON) drawText("OFF", W - 34, y, GUI_THEME.footer_size, GUI_THEME.footer_active.r, GUI_THEME.footer_active.g, GUI_THEME.footer_active.b);
    else                     drawText("OFF", W - 34, y, GUI_THEME.footer_size, 255, 0, 0);
  }
}

static void renderFRQ(int16_t W, int16_t H) {
  uint8_t valueSize = GUI_THEME.value_size;
  uint8_t unitSize  = GUI_THEME.unit_size;
  int gapPx = 2 * valueSize;

  char frqStr[8];
  formatFreq(frqStr);
  const char* unit = "MHz";

  int valueWidth = textW(frqStr, valueSize);
  int unitWidth  = textW(unit, unitSize);
  int totalWidth = valueWidth + gapPx + unitWidth;

  int startX = (W - totalWidth) / 2;
  int y = (H / 2) - (textH(valueSize) / 2);

  drawText(frqStr, startX, y, valueSize, GUI_THEME.value_text.r, GUI_THEME.value_text.g, GUI_THEME.value_text.b);

  int unitX = startX + valueWidth + gapPx;
  int unitY = y + textH(valueSize) - textH(unitSize);
  drawText(unit, unitX, unitY, unitSize, GUI_THEME.unit_text.r, GUI_THEME.unit_text.g, GUI_THEME.unit_text.b);

  if (ui.edit) {
    int charIndex = (ui.cursor <= 2) ? ui.cursor : (ui.cursor + 1);
    int charWpx = 6 * valueSize;
    int charHpx = 8 * valueSize;

    int underlineX0 = startX + charIndex * charWpx;
    int underlineX1 = underlineX0 + charWpx - 2;
    int underlineY  = y + charHpx + valueSize;

    lineCursor(underlineX0, underlineY, underlineX1, underlineY);
  }
}

static void renderListValue(int16_t W, int16_t H, const char* value) {
  uint8_t size = GUI_THEME.value_size + 1;
  int w = textW(value, size);
  int h = textH(size);

  int x = (W - w) / 2;
  int y = (H / 2) - (h / 2);

  drawText(value, x, y, size, GUI_THEME.value_text.r, GUI_THEME.value_text.g, GUI_THEME.value_text.b);

  if (ui.edit) {
    int underlineY = y + h + size;
    lineCursor(x, underlineY, x + w - 2, underlineY);
  }
}

static void renderValueArea(int16_t W, int16_t H) {
  int y0 = GUI_LIMITS.header_h;
  int h  = H - GUI_LIMITS.header_h - GUI_LIMITS.footer_h;
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

static void renderDirty() {
  int16_t W, H;
  getDisplaySize(W, H);

  if (dirtyHeader) { renderHeaderArea(W); dirtyHeader = false; }
  if (dirtyValue)  { renderValueArea(W, H); dirtyValue = false; }
  if (dirtyFooter) { renderFooterArea(W, H); dirtyFooter = false; }
}

// ---------- State transitions ----------
static void enterEdit() {
  ui.edit = true;
  ui.cursor = 0;
}

static void saveCurrentValue() {
  // Save nur sinnvoll, wenn edit aktiv war (Cursor sichtbar)
  ui.edit = false;
  ui.cursor = 0;
  ui.toastUntil = millis() + GUI_LIMITS.toast_ms;

  ui.saveRequested = true;
  ui.savedScreen = ui.screen;
}

static void nextCursorPosition() {
  if (ui.screen == GUI_FRQ) ui.cursor = (ui.cursor + 1) % 6;
  else ui.cursor = 0;
}

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

static void focusSet(GuiFooterItem f) {
  ui.focus = f;

  // Wenn Fokus auf FRQ/MOD/PWR: Screen folgt dem Fokus.
  if (ui.focus == FOOT_FRQ) ui.screen = GUI_FRQ;
  else if (ui.focus == FOOT_MOD) ui.screen = GUI_MOD;
  else if (ui.focus == FOOT_PWR) ui.screen = GUI_PWR;

  // Edit wird beendet, wenn man im Footer navigiert
  ui.edit = false;
  ui.cursor = 0;
}

// ---------- Public ----------
void guiInit() {
  ui = UIState{};
  ui.screen = GUI_FRQ;
  ui.focus  = FOOT_FRQ;

  freq_hz  = GUI_DEFAULTS.frq_start_hz;
  modIndex = modPos(GUI_DEFAULTS.mod_index, GUI_MOD_COUNT);
  pwrIndex = modPos(GUI_DEFAULTS.pwr_index, GUI_PWR_COUNT);
  limitFreq();

  initialized = true;

  clearDisplay();
  dirtyHeader = dirtyValue = dirtyFooter = true;
  renderDirty();
}

void guiUpdate() {
  if (!initialized) return;

  updateRotaryEncoder();
  updateNavButtons();

  // Footer-Navigation (nur wenn nicht im Edit)
  if (!ui.edit) {
    if (getLeftPressed()) {
      int f = (int)ui.focus - 1;
      if (f < 0) f = 3;
      focusSet((GuiFooterItem)f);
      dirtyHeader = dirtyValue = dirtyFooter = true;
    }
    if (getRightPressed()) {
      int f = (int)ui.focus + 1;
      if (f > 3) f = 0;
      focusSet((GuiFooterItem)f);
      dirtyHeader = dirtyValue = dirtyFooter = true;
    }
  }

  // Encoder Short: abhängig vom Footer-Fokus
  if (getButtonPressed()) {
    if (!ui.edit) {
      if (ui.focus == FOOT_ON) {
        // ON wird "gedrückt": Event an main.cpp
        ui.onToggleRequested = true;
      } else {
        // FRQ/MOD/PWR: Edit starten (Cursor sichtbar)
        enterEdit();
        dirtyValue = true;
      }
    } else {
      // Edit aktiv: Cursor weiter (bei FRQ)
      nextCursorPosition();
      dirtyValue = true;
    }
  }

  // Encoder drehen: nur im Edit
  int32_t d = getEncoderDelta();
  if (d != 0 && ui.edit) {
    changeValueByDelta(d);
    dirtyValue = true;
  }

  // Encoder Long: Save (nur wenn Edit aktiv, damit kein “Ghost Save” passiert)
  if (getButtonLongPressed()) {
    if (ui.edit) {
      saveCurrentValue();
      dirtyHeader = true;
      dirtyValue  = true;
    }
  }

  // Toast Ablauf -> Header normal neu zeichnen
  static bool toastWasActive = false;
  bool toastActive = (millis() < ui.toastUntil);
  if (toastWasActive && !toastActive) dirtyHeader = true;
  toastWasActive = toastActive;

  if (dirtyHeader || dirtyValue || dirtyFooter) renderDirty();
}

void guiForceRedraw() {
  dirtyHeader = dirtyValue = dirtyFooter = true;
  renderDirty();
}

GuiScreen guiGetScreen() { return ui.screen; }
bool guiIsEditing() { return ui.edit; }

void guiSetRadioOn(bool on) {
  if (ui.radioOn != on) {
    ui.radioOn = on;
    dirtyFooter = true;
  }
}
bool guiGetRadioOn() { return ui.radioOn; }

bool guiConsumeOnToggleRequested() {
  bool v = ui.onToggleRequested;
  ui.onToggleRequested = false;
  return v;
}

bool guiConsumeSaveRequested(GuiScreen &screen) {
  if (!ui.saveRequested) return false;
  ui.saveRequested = false;
  screen = ui.savedScreen;
  return true;
}

int32_t guiGetFrequencyHz() { return freq_hz; }
int guiGetModIndex() { return modIndex; }
int guiGetPwrIndex() { return pwrIndex; }
