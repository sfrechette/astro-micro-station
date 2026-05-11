#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include "config.h"

// ═══════════════════════════════════════════════════════════════════════════════
//  AstroAPI.h  –  Data structures and class declaration
//  Implementation: src/AstroAPI.cpp
// ═══════════════════════════════════════════════════════════════════════════════

// ─── Twilight block (morning or evening) ──────────────────────────────────────
struct TwilightTimes {
    String astronomicalBegin;
    String astronomicalEnd;
    String nauticalBegin;
    String nauticalEnd;
    String civilBegin;
    String civilEnd;
    String blueHourBegin;
    String blueHourEnd;
    String goldenHourBegin;
    String goldenHourEnd;
};

// ─── Full astronomy response model ────────────────────────────────────────────
struct AstroData {
    bool          valid            = false;
    String        fetchDate;
    String        currentTime;

    // Sun
    String        sunrise;
    String        sunset;
    String        solarNoon;
    String        dayLength;
    float         sunAltitude      = 0.0f;
    float         sunAzimuth       = 0.0f;
    TwilightTimes morning;
    TwilightTimes evening;
    String        nightBegin;
    String        nightEnd;

    // Sun extended
    float         sunDistance      = 0.0f;   // km
    String        midNight;

    // Moon
    String        moonPhase;
    float         moonIllumination = 0.0f;   // 0 – 100 %
    String        moonrise;
    String        moonset;
    float         moonAltitude          = 0.0f;
    float         moonAzimuth           = 0.0f;
    float         moonAngle             = 0.0f;
    float         moonDistance          = 0.0f;   // km
};

// ─── Utility free functions ───────────────────────────────────────────────────
String moonPhaseName(const String& raw);

// ═══════════════════════════════════════════════════════════════════════════════
//  AstroAPI class
// ═══════════════════════════════════════════════════════════════════════════════
class AstroAPI {
public:
    AstroData data;

    bool fetch();
    bool loadCache();   // parse last saved JSON from LittleFS; returns false if none

private:
    void parseTwilight(JsonObject obj, TwilightTimes& out);
    bool parseResponse(const String& body);  // shared parser used by fetch + loadCache
};
