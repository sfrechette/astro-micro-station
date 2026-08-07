#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>
#include "config.h"
#include "AstroAPI.h"

// ═══════════════════════════════════════════════════════════════════════════════
//  DisplayManager.h  –  Screen rendering and touch input declarations
//  Implementation: src/DisplayManager.cpp
//
//  Screen index map:
//    0 = Moon Detail
//    1 = Day / Sun
//    2 = Morning Twilight
//    3 = Evening Twilight
// ═══════════════════════════════════════════════════════════════════════════════

static constexpr int DISP_W = 480;
static constexpr int DISP_H = 222;

// ─── Colour palette ───────────────────────────────────────────────────────────
namespace Pal {
    // Normal mode
    constexpr uint16_t BG     = 0x0841;
    constexpr uint16_t ACCENT = 0x05FF;
    constexpr uint16_t WHITE  = 0xFFFF;
    constexpr uint16_t SILVER = 0xC618; // #C0C0C0
    constexpr uint16_t GREY   = 0x8410;
    constexpr uint16_t RED    = 0xF800;
    constexpr uint16_t GOLD   = 0xFEA0;
    constexpr uint16_t DKGREEN = 0x5D5D; // light-medium blue #5AAAEA
    constexpr uint16_t CARD     = 0x0841; // barely-above-black card background
    constexpr uint16_t R_CARD   = 0x0800; // barely-above-black red card (night mode)
    constexpr uint16_t CARD_LBL = 0x94B2; // card label text  RGB(150,150,150)
    constexpr uint16_t CARD_VAL = 0xAD55; // card value text  RGB(170,170,170)
    // Red / night-vision mode
    constexpr uint16_t R_BG   = 0x2000;
    constexpr uint16_t R_TEXT = 0xFBC0;
    constexpr uint16_t R_DIM  = 0x8000;
}

// ═══════════════════════════════════════════════════════════════════════════════
class DisplayManager {
public:
    TFT_eSPI    tft;
    TFT_eSprite sprite = TFT_eSprite(&tft);

    bool redMode = false;
    bool dimmed  = false;

    // Initialise TFT, backlight PWM and sprite buffer.
    void begin();

    // Set backlight brightness (0–255).
    void setBacklight(uint8_t brightness);

    // Toggle red night-vision mode.
    void setRedMode(bool on);

    // Draw the requested screen index (0=Morning, 1=Evening, 2=Day, 3=Moon) into sprite and push.
    void drawScreen(int screen, const AstroData& astro);

private:
    // ── Per-screen draw methods ───────────────────────────────────────────────
    void drawMorning(const AstroData& a);
    void drawEvening(const AstroData& a);
    void drawDay(const AstroData& a);
    void drawMoon(const AstroData& a);
    void drawPageHeader(const char* title, const AstroData& a);
    void drawNavDots(int active);
    // label always font1; valFont defaults to 2, pass 4 for large time strings
    void drawCard(int x, int y, int w, int h, const char* lbl, const String& val, uint16_t valCol, uint8_t valFont = 2);

    // ── Colour helpers ────────────────────────────────────────────────────────
    uint16_t accentColor() const { return redMode ? Pal::R_TEXT : Pal::ACCENT; }
    uint16_t bgColor()     const { return redMode ? Pal::R_BG   : Pal::BG;     }
    uint16_t cardBg()      const { return redMode ? Pal::R_CARD : Pal::CARD;   }
};
