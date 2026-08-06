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

    // Header label — reverse-geocoded "City, ST" from the API's own location
    // block, refreshed on every successful parseResponse(). Not user-entered.
    String        observerName     = "Gatineau, QC";
};

// ─── Utility free functions ───────────────────────────────────────────────────
String moonPhaseName(const String& raw);

// ═══════════════════════════════════════════════════════════════════════════════
//  AstroAPI class
// ═══════════════════════════════════════════════════════════════════════════════
class AstroAPI {
public:
    AstroData data;

    void begin();        // loads a persisted observer-location override, if any
    bool fetch();
    bool loadCache();    // parse last saved JSON from LittleFS; returns false if none

    float observerLat() const { return lat_; }
    float observerLon() const { return lon_; }

    // Validates range, persists to LittleFS, and updates the location used by
    // the next fetch(). Returns false (no change made) if out of range.
    bool setLocation(float lat, float lon);

    // Resets to OBSERVER_LAT/OBSERVER_LON from secrets.h — the "Use home
    // location" button. Those coordinates never need to pass through or be
    // stored in the (non-gitignored) dashboard page this way.
    bool useHomeLocation() { return setLocation(OBSERVER_LAT, OBSERVER_LON); }

private:
    float lat_ = OBSERVER_LAT;
    float lon_ = OBSERVER_LON;

    void parseTwilight(JsonObject obj, TwilightTimes& out);
    bool parseResponse(const String& body);  // shared parser used by fetch + loadCache
};
