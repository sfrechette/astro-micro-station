#include "AstroAPI.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

// ═══════════════════════════════════════════════════════════════════════════════
//  AstroAPI.cpp  –  ipgeolocation.io /astronomy fetch + parse
// ═══════════════════════════════════════════════════════════════════════════════

static constexpr const char* CACHE_PATH = "/astro_cache.json";

// ─── Free functions ───────────────────────────────────────────────────────────

String moonPhaseName(const String& raw) {
    if (raw == "NEW_MOON")        return "New Moon";
    if (raw == "WAXING_CRESCENT") return "Waxing Crescent";
    if (raw == "FIRST_QUARTER")   return "First Quarter";
    if (raw == "WAXING_GIBBOUS")  return "Waxing Gibbous";
    if (raw == "FULL_MOON")       return "Full Moon";
    if (raw == "WANING_GIBBOUS")  return "Waning Gibbous";
    if (raw == "LAST_QUARTER")    return "Last Quarter";
    if (raw == "WANING_CRESCENT") return "Waning Crescent";
    return raw;
}

// ─── Private: parseTwilight ───────────────────────────────────────────────────

void AstroAPI::parseTwilight(JsonObject obj, TwilightTimes& out) {
    out.astronomicalBegin = obj["astronomical_twilight_begin"] | "--:--";
    out.astronomicalEnd   = obj["astronomical_twilight_end"]   | "--:--";
    out.nauticalBegin     = obj["nautical_twilight_begin"]     | "--:--";
    out.nauticalEnd       = obj["nautical_twilight_end"]       | "--:--";
    out.civilBegin        = obj["civil_twilight_begin"]        | "--:--";
    out.civilEnd          = obj["civil_twilight_end"]          | "--:--";
    out.blueHourBegin     = obj["blue_hour_begin"]             | "--:--";
    out.blueHourEnd       = obj["blue_hour_end"]               | "--:--";
    out.goldenHourBegin   = obj["golden_hour_begin"]           | "--:--";
    out.goldenHourEnd     = obj["golden_hour_end"]             | "--:--";
}

// ─── Private: parseResponse ───────────────────────────────────────────────────
// Shared by fetch() and loadCache(). Returns true and populates data on success.

bool AstroAPI::parseResponse(const String& body) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, body);
    if (err) {
        Serial.printf("[AstroAPI] JSON parse error: %s\n", err.c_str());
        return false;
    }

    JsonObject astro = doc["astronomy"].as<JsonObject>();
    if (astro.isNull() || !astro.containsKey("sunrise")) {
        Serial.println("[AstroAPI] Unexpected response structure");
        return false;
    }

    data.fetchDate    = astro["date"]         | "";
    data.currentTime  = astro["current_time"] | "";
    data.sunrise      = astro["sunrise"]      | "--:--";
    data.sunset       = astro["sunset"]       | "--:--";
    data.solarNoon    = astro["solar_noon"]   | "--:--";
    data.dayLength    = astro["day_length"]   | "--:--";
    data.sunAltitude  = astro["sun_altitude"] | 0.0f;
    data.sunAzimuth   = astro["sun_azimuth"]  | 0.0f;

    parseTwilight(astro["morning"], data.morning);
    parseTwilight(astro["evening"], data.evening);

    data.nightEnd   = astro["night_end"]   | "--:--";
    data.nightBegin = astro["night_begin"] | "--:--";
    data.sunDistance = astro["sun_distance"] | 0.0f;
    data.midNight    = astro["mid_night"]    | "--:--";

    auto cleanTime = [](const String& s) -> String {
        return (s == "-:-" || s == "") ? "--:--" : s;
    };

    data.moonPhase            = astro["moon_phase"]                                     | "UNKNOWN";
    data.moonIllumination     = fabsf(astro["moon_illumination_percentage"].as<String>().toFloat());
    data.moonrise             = cleanTime(astro["moonrise"] | "-:-");
    data.moonset              = cleanTime(astro["moonset"]  | "-:-");
    data.moonAltitude         = astro["moon_altitude"]         | 0.0f;
    data.moonAzimuth          = astro["moon_azimuth"]          | 0.0f;
    data.moonAngle            = astro["moon_angle"]            | 0.0f;
    data.moonDistance         = astro["moon_distance"]         | 0.0f;
    data.valid = true;

    Serial.printf("[AstroAPI] Parsed OK — %s  illum=%.1f%%  sun=%.1f°\n",
                  data.moonPhase.c_str(), data.moonIllumination, data.sunAltitude);
    return true;
}

// ─── Public: fetch ────────────────────────────────────────────────────────────

bool AstroAPI::fetch() {
    String url = "https://api.ipgeolocation.io/v2/astronomy"
                 "?apiKey=" + String(ASTRO_API_KEY) +
                 "&lat="    + String(OBSERVER_LAT, 6) +
                 "&long="   + String(OBSERVER_LON, 6) +
                 "&elevation=10";

    Serial.println("[AstroAPI] Fetching ipgeolocation.io/v2/astronomy ...");

    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    http.begin(client, url);
    http.setTimeout(15000);

    uint32_t t0 = millis();
    int code = http.GET();
    Serial.printf("[AstroAPI] HTTP %d  (%lu ms)\n", code, (unsigned long)(millis() - t0));

    if (code != 200) {
        Serial.printf("[AstroAPI] Error body: %.300s\n", http.getString().c_str());
        http.end();
        return false;
    }

    String body = http.getString();
    http.end();
    Serial.printf("[AstroAPI] Response size: %d bytes\n", body.length());

    if (!parseResponse(body))
        return false;

    // Save to LittleFS so next boot can use it offline
    File f = LittleFS.open(CACHE_PATH, "w");
    if (f) {
        f.print(body);
        f.close();
        Serial.printf("[AstroAPI] Cache saved (%d bytes)\n", body.length());
    } else {
        Serial.println("[AstroAPI] Cache write failed");
    }
    return true;
}

// ─── Public: loadCache ────────────────────────────────────────────────────────

bool AstroAPI::loadCache() {
    if (!LittleFS.exists(CACHE_PATH)) {
        Serial.println("[AstroAPI] No cache file found");
        return false;
    }
    File f = LittleFS.open(CACHE_PATH, "r");
    if (!f) {
        Serial.println("[AstroAPI] Cache open failed");
        return false;
    }
    String body = f.readString();
    f.close();
    Serial.printf("[AstroAPI] Cache loaded (%d bytes) — parsing...\n", body.length());
    return parseResponse(body);
}
