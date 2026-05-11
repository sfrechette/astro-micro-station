#include "DisplayManager.h"
#include "icons.h"
#include "moon_icons.h"
#include <math.h>

// Render a float with up to 6 decimal places, trailing zeros stripped
static String trimFloat(float v) {
    String s = String(v, 6);
    if (s.indexOf('.') >= 0) {
        while (s.endsWith("0")) s.remove(s.length() - 1);
        if (s.endsWith("."))   s.remove(s.length() - 1);
    }
    return s;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  DisplayManager.cpp  –  All screen rendering and touch input
// ═══════════════════════════════════════════════════════════════════════════════

// ─── drawCard ─────────────────────────────────────────────────────────────────
// Shaded rounded tile: dim label (font1) above coloured value (valFont).
// Content is vertically centred inside the card.

void DisplayManager::drawCard(int x, int y, int w, int h,
                               const char* lbl, const String& val,
                               uint16_t valCol, uint8_t valFont) {
    sprite.fillSmoothRoundRect(x, y, w, h, 4, cardBg(), 0x0000);
    const int lblH = 8;                          // font1 height
    const int valH = (valFont == 4) ? 26 : 16;  // font4 or font2 height
    int yStart = y + (h - lblH - 2 - valH) / 2;
    int cx = x + w / 2;
    sprite.setTextDatum(TC_DATUM);
    sprite.setTextColor(redMode ? Pal::R_TEXT : Pal::CARD_LBL);
    sprite.drawString(lbl, cx, yStart, 1);
    sprite.setTextColor(redMode ? Pal::R_TEXT : Pal::WHITE);
    sprite.drawString(val, cx, yStart + lblH + 2, valFont);
    sprite.setTextDatum(TL_DATUM);
}

// ─── begin ────────────────────────────────────────────────────────────────────

void DisplayManager::begin() {
    tft.init();
    tft.setRotation(1);       // landscape, USB on left
    tft.fillScreen(Pal::BG);
    tft.setSwapBytes(true);

    ledcAttach(TFT_BL, 5000, 8);
    setBacklight(BACKLIGHT_NORMAL);

    sprite.setColorDepth(16);
    sprite.createSprite(DISP_W, DISP_H);

    Serial.println("[Display] Init OK — 480×222");
    Serial.printf("[Mem]  Heap free: %lu  min: %lu  maxBlock: %lu\n",
                  ESP.getFreeHeap(), ESP.getMinFreeHeap(), ESP.getMaxAllocHeap());
    Serial.printf("[Mem]  PSRAM free: %lu / %lu  (sprite buffer: %d bytes)\n",
                  ESP.getFreePsram(), ESP.getPsramSize(), DISP_W * DISP_H * 2);
}

// ─── setBacklight ─────────────────────────────────────────────────────────────

void DisplayManager::setBacklight(uint8_t brightness) {
    ledcWrite(TFT_BL, brightness);
}

// ─── setRedMode ───────────────────────────────────────────────────────────────

void DisplayManager::setRedMode(bool on) {
    redMode = on;
    Serial.printf("[Display] Red mode: %s\n", on ? "ON" : "OFF");
}

// ─── drawScreen ───────────────────────────────────────────────────────────────

void DisplayManager::drawScreen(int screen, const AstroData& astro, time_t now) {
    sprite.fillSprite(bgColor());                              // header band gets bg colour
    sprite.fillRect(0, 27, DISP_W, DISP_H - 27, 0x0000);    // content area pure black

    switch (screen) {
        case 0: drawMoon(astro);    break;
        case 1: drawDay(astro);     break;
        case 2: drawMorning(astro); break;
        case 3: drawEvening(astro); break;
    }

    // ── Bottom status bar (mirrors top header) ────────────────────────────────
    const int barH  = 26;
    const int barY  = DISP_H - barH;                              // y = 196
    sprite.fillRect(0, barY, DISP_W, barH, redMode ? 0x2000 : 0x0041);
    sprite.drawLine(0, barY, DISP_W, barY, redMode ? Pal::R_DIM : 0x4208);

    // Nav dots sit on top of the bar fill
    drawNavDots(screen);

    uint16_t barCol = redMode ? Pal::R_TEXT : Pal::DKGREEN;
    const int yBar  = barY + 9;   // vertically centred inside the 26px band

    // Left: date from API
    sprite.setTextColor(barCol);
    sprite.setTextDatum(TL_DATUM);
    if (astro.valid && astro.fetchDate.length() > 0)
        sprite.drawString(astro.fetchDate, 10, yBar, 1);

    // Right: time of last API fetch
    if (astro.valid && astro.currentTime.length() > 0) {
        sprite.setTextDatum(TR_DATUM);
        sprite.drawString(astro.currentTime, DISP_W - 10, yBar, 1);
        sprite.setTextDatum(TL_DATUM);
    }

    sprite.pushSprite(0, 0);
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Shared header — used by all 4 screens
// ═══════════════════════════════════════════════════════════════════════════════

void DisplayManager::drawPageHeader(const char* title, const AstroData& a) {
    sprite.fillRect(0, 0, DISP_W, 26, redMode ? 0x2000 : 0x0041);
    sprite.drawLine(0, 26, DISP_W, 26, redMode ? Pal::R_DIM : 0x4208);

    // Icon — drawn in original colours normally, tinted R_TEXT in red mode
    const int iconSize = HEADER_ICON_SIZE;
    const int iconX    = 4;
    const int iconY    = (26 - iconSize) / 2;
    {
        const uint16_t* src = HEADER_ICON;
        for (int row = 0; row < iconSize; row++) {
            for (int col = 0; col < iconSize; col++) {
                uint16_t c = pgm_read_word(src++);
                if (c != 0xFFFF)
                    sprite.drawPixel(iconX + col, iconY + row, redMode ? Pal::R_TEXT : c);
            }
        }
    }
    const int textX = iconX + iconSize + 4;

    // App name — green, pseudo-bold
    uint16_t appCol  = redMode ? Pal::R_TEXT : Pal::DKGREEN;
    uint16_t bktCol  = redMode ? Pal::R_TEXT : Pal::CARD_LBL;
    uint16_t pageCol = redMode ? Pal::R_TEXT : Pal::WHITE;
    sprite.setTextColor(appCol);
    sprite.drawString("ASTRONOMY MICRO STATION", textX, 5, 2);
    sprite.drawString("ASTRONOMY MICRO STATION", textX + 1, 5, 2);
    int appW = sprite.textWidth("ASTRONOMY MICRO STATION", 2);

    // Page title — brackets green, title white, pseudo-bold
    int px = textX + appW + 8;
    sprite.setTextColor(bktCol);
    sprite.drawString("[", px, 5, 2);
    sprite.drawString("[", px + 1, 5, 2);
    int bracketW = sprite.textWidth("[", 2);

    sprite.setTextColor(pageCol);
    sprite.drawString(title, px + bracketW, 5, 2);
    sprite.drawString(title, px + bracketW + 1, 5, 2);
    int titleW = sprite.textWidth(title, 2);

    sprite.setTextColor(bktCol);
    sprite.drawString("]", px + bracketW + titleW, 5, 2);
    sprite.drawString("]", px + bracketW + titleW + 1, 5, 2);

    // Location right-aligned, green bold
    sprite.setTextColor(appCol);
    sprite.setTextDatum(TR_DATUM);
    sprite.drawString("Gatineau, QC", DISP_W - 10, 5, 2);
    sprite.drawString("Gatineau, QC", DISP_W - 11, 5, 2);
    sprite.setTextDatum(TL_DATUM);
}

// ═══════════════════════════════════════════════════════════════════════════════
//  SCREEN 0 · Morning twilight
// ═══════════════════════════════════════════════════════════════════════════════

void DisplayManager::drawMorning(const AstroData& a) {
    drawPageHeader("MORNING", a);

    if (!a.valid) {
        sprite.setTextColor(Pal::RED);
        sprite.drawString("Waiting for data...", 10, 80, 2);
        return;
    }

    // Cards: w=228 h=50, 4px gap — left col 3 items, right col 2 items
    const int cW = 228, cH = 50, cGap = 4;
    const int xL = 6, xR = 246, y0 = 31;

    struct { const char* label; String val; uint16_t col; } items[] = {
        { "ASTRONOMY TWILIGHT", a.morning.astronomicalBegin + " - " + a.morning.astronomicalEnd, accentColor()   },
        { "NAUTICAL TWILIGHT",  a.morning.nauticalBegin     + " - " + a.morning.nauticalEnd,     Pal::CARD_VAL  },
        { "CIVIL TWILIGHT",     a.morning.civilBegin        + " - " + a.morning.civilEnd,         Pal::CARD_LBL  },
        { "BLUE HOUR",          a.morning.blueHourBegin     + " - " + a.morning.blueHourEnd,      0x07FF         },
        { "GOLDEN HOUR",        a.morning.goldenHourBegin   + " - " + a.morning.goldenHourEnd,    Pal::GOLD      },
    };

    for (int i = 0; i < 3; i++)
        drawCard(xL, y0 + i * (cH + cGap), cW, cH, items[i].label, items[i].val, items[i].col, 4);
    for (int i = 3; i < 5; i++)
        drawCard(xR, y0 + (i - 3) * (cH + cGap), cW, cH, items[i].label, items[i].val, items[i].col, 4);
}

// ═══════════════════════════════════════════════════════════════════════════════
//  SCREEN 1 · Evening twilight
// ═══════════════════════════════════════════════════════════════════════════════

void DisplayManager::drawEvening(const AstroData& a) {
    drawPageHeader("EVENING", a);

    if (!a.valid) {
        sprite.setTextColor(Pal::RED);
        sprite.drawString("Waiting for data...", 10, 80, 2);
        return;
    }

    const int cW = 228, cH = 50, cGap = 4;
    const int xL = 6, xR = 246, y0 = 31;

    struct { const char* label; String val; uint16_t col; } items[] = {
        { "GOLDEN HOUR",        a.evening.goldenHourBegin   + " - " + a.evening.goldenHourEnd,   Pal::GOLD     },
        { "BLUE HOUR",          a.evening.blueHourBegin     + " - " + a.evening.blueHourEnd,     0x07FF        },
        { "CIVIL TWILIGHT",     a.evening.civilBegin        + " - " + a.evening.civilEnd,         Pal::CARD_LBL },
        { "NAUTICAL TWILIGHT",  a.evening.nauticalBegin     + " - " + a.evening.nauticalEnd,     Pal::CARD_VAL },
        { "ASTRONOMY TWILIGHT", a.evening.astronomicalBegin + " - " + a.evening.astronomicalEnd, accentColor() },
    };

    for (int i = 0; i < 3; i++)
        drawCard(xL, y0 + i * (cH + cGap), cW, cH, items[i].label, items[i].val, items[i].col, 4);
    for (int i = 3; i < 5; i++)
        drawCard(xR, y0 + (i - 3) * (cH + cGap), cW, cH, items[i].label, items[i].val, items[i].col, 4);
}

// ═══════════════════════════════════════════════════════════════════════════════
//  SCREEN 2 · Day / Sun
// ═══════════════════════════════════════════════════════════════════════════════

void DisplayManager::drawDay(const AstroData& a) {
    drawPageHeader("DAY", a);

    if (!a.valid) {
        sprite.setTextColor(Pal::RED);
        sprite.drawString("Waiting for data...", 10, 80, 2);
        return;
    }

    const int cW = 228, cH = 29, cGap = 4;
    const int xL = 6, xR = 246, y0 = 31;

    struct { const char* label; String val; uint16_t col; } left[] = {
        { "SUNRISE",      a.sunrise,                                   Pal::GOLD                                        },
        { "SUNSET",       a.sunset,                                    Pal::GOLD                                        },
        { "SOLAR NOON",   a.solarNoon,                                 accentColor()                                    },
        { "DAY LENGTH",   a.dayLength,                                 Pal::CARD_VAL                                        },
        { "SUN ALTITUDE", trimFloat(a.sunAltitude) + "\xb0",           a.sunAltitude >= 0.0f ? Pal::CARD_VAL : Pal::CARD_LBL },
    };
    struct { const char* label; String val; uint16_t col; } right[] = {
        { "SUN AZIMUTH", trimFloat(a.sunAzimuth) + "\xb0",          Pal::CARD_VAL },
        { "DISTANCE",    String(a.sunDistance / 1e6f, 1) + "M km",  Pal::CARD_LBL },
        { "MID NIGHT",   a.midNight,                                 accentColor() },
        { "NIGHT END",   a.nightEnd,                                 accentColor() },
        { "NIGHT BEGIN", a.nightBegin,                               accentColor() },
    };

    for (int i = 0; i < 5; i++)
        drawCard(xL, y0 + i * (cH + cGap), cW, cH, left[i].label,  left[i].val,  left[i].col);
    for (int i = 0; i < 5; i++)
        drawCard(xR, y0 + i * (cH + cGap), cW, cH, right[i].label, right[i].val, right[i].col);
}

// ═══════════════════════════════════════════════════════════════════════════════
//  SCREEN 3 · Moon Detail
// ═══════════════════════════════════════════════════════════════════════════════

void DisplayManager::drawMoon(const AstroData& a) {
    drawPageHeader("MOON", a);

    if (!a.valid) {
        sprite.setTextColor(Pal::RED); sprite.drawString("No data", 10, 80, 2);
        return;
    }

    // ── Moon phase image (left 162px area, centred) ───────────────────────────
    {
        const int imgX = (162 - MOON_IMG_W) / 2;          // ≈ 3
        const int imgY = 27 + (169 - MOON_IMG_H) / 2;     // ≈ 34
        const uint16_t* src = MOON_IMGS[moonPhaseIndex(a.moonPhase)];
        for (int row = 0; row < MOON_IMG_H; row++) {
            for (int col = 0; col < MOON_IMG_W; col++) {
                uint16_t c = pgm_read_word(src++);
                if (c == 0xFFFF) continue;
                if (redMode) {
                    uint8_t lum = (c >> 11) & 0x1F;  // 5-bit luminance from red channel
                    c = (uint16_t)(lum << 11);        // red-only, preserves shading
                }
                sprite.drawPixel(imgX + col, imgY + row, c);
            }
        }
    }

    // ── Data panel cards: 2-column × 4-row grid ───────────────────────────────
    const int cW = 151, cH = 37, cGap = 5;
    const int xL = 168, xR = 323, y0 = 31;

    struct { const char* label; String val; uint16_t col; } left[] = {
        { "PHASE",    moonPhaseName(a.moonPhase),                     Pal::CARD_VAL },
        { "MOONRISE", a.moonrise,                                     Pal::CARD_VAL },
        { "ALTITUDE", trimFloat(a.moonAltitude) + "\xb0",            Pal::CARD_VAL },
        { "DISTANCE", String(a.moonDistance / 1000.0f, 0) + "k km", Pal::CARD_LBL },
    };
    struct { const char* label; String val; uint16_t col; } right[] = {
        { "ILLUMINATION", String((int)round(a.moonIllumination)) + "%", Pal::WHITE    },
        { "MOONSET",      a.moonset,                                    Pal::CARD_VAL },
        { "AZIMUTH",      trimFloat(a.moonAzimuth) + "\xb0",           Pal::CARD_VAL },
        { "ANGLE",        trimFloat(a.moonAngle)   + "\xb0",           Pal::CARD_VAL },
    };

    for (int i = 0; i < 4; i++)
        drawCard(xL, y0 + i * (cH + cGap), cW, cH, left[i].label,  left[i].val,  left[i].col);
    for (int i = 0; i < 4; i++)
        drawCard(xR, y0 + i * (cH + cGap), cW, cH, right[i].label, right[i].val, right[i].col);
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Navigation dots
// ═══════════════════════════════════════════════════════════════════════════════

void DisplayManager::drawNavDots(int active) {
    const int total = 4, dotR = 4, gap = 16;
    int startX = DISP_W / 2 - (total * gap / 2);
    int y = DISP_H - 10;
    for (int i = 0; i < total; i++) {
        int x = startX + i * gap;
        if (i == active)
            sprite.fillCircle(x, y, dotR, redMode ? Pal::R_TEXT : Pal::DKGREEN);
        else
            sprite.drawCircle(x, y, dotR, redMode ? Pal::R_DIM : 0x4208);
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Touch FSM
// ═══════════════════════════════════════════════════════════════════════════════

int DisplayManager::processTouchEvent(uint16_t x, uint16_t y, bool down, uint32_t ms) {
    if (down) {
        if (!_touched) {
            _touched      = true;
            _touchStartMs = ms;
        }
    } else {
        if (_touched) {
            _touched = false;
            if (ms - _touchStartMs < 600)
                return +1;  // tap → next page
        }
    }
    return 0;
}
