#include "AstroAPI.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

// ═══════════════════════════════════════════════════════════════════════════════
//  AstroAPI.cpp  –  ipgeolocation.io /astronomy fetch + parse
// ═══════════════════════════════════════════════════════════════════════════════

static constexpr const char* CACHE_PATH    = "/astro_cache.json";
static constexpr const char* LOCATION_PATH = "/location.json";

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

// ─── Private: abbreviateRegion ────────────────────────────────────────────────
// ipgeolocation.io returns state_prov as a full name ("Quebec"); the header
// needs the short form ("QC"). Falls back to the full name for anything not
// in the table (i.e. outside Canada/US), so it degrades rather than breaks.

static String abbreviateRegion(const String& full) {
    static const struct { const char* full; const char* abbr; } TABLE[] = {
        {"Alberta","AB"},{"British Columbia","BC"},{"Manitoba","MB"},{"New Brunswick","NB"},
        {"Newfoundland and Labrador","NL"},{"Northwest Territories","NT"},{"Nova Scotia","NS"},
        {"Nunavut","NU"},{"Ontario","ON"},{"Prince Edward Island","PE"},{"Quebec","QC"},
        {"Saskatchewan","SK"},{"Yukon","YT"},
        {"Alabama","AL"},{"Alaska","AK"},{"Arizona","AZ"},{"Arkansas","AR"},{"California","CA"},
        {"Colorado","CO"},{"Connecticut","CT"},{"Delaware","DE"},{"Florida","FL"},{"Georgia","GA"},
        {"Hawaii","HI"},{"Idaho","ID"},{"Illinois","IL"},{"Indiana","IN"},{"Iowa","IA"},
        {"Kansas","KS"},{"Kentucky","KY"},{"Louisiana","LA"},{"Maine","ME"},{"Maryland","MD"},
        {"Massachusetts","MA"},{"Michigan","MI"},{"Minnesota","MN"},{"Mississippi","MS"},
        {"Missouri","MO"},{"Montana","MT"},{"Nebraska","NE"},{"Nevada","NV"},{"New Hampshire","NH"},
        {"New Jersey","NJ"},{"New Mexico","NM"},{"New York","NY"},{"North Carolina","NC"},
        {"North Dakota","ND"},{"Ohio","OH"},{"Oklahoma","OK"},{"Oregon","OR"},{"Pennsylvania","PA"},
        {"Rhode Island","RI"},{"South Carolina","SC"},{"South Dakota","SD"},{"Tennessee","TN"},
        {"Texas","TX"},{"Utah","UT"},{"Vermont","VT"},{"Virginia","VA"},{"Washington","WA"},
        {"West Virginia","WV"},{"Wisconsin","WI"},{"Wyoming","WY"},{"District of Columbia","DC"},
    };
    for (const auto& row : TABLE) {
        if (full.equalsIgnoreCase(row.full)) return String(row.abbr);
    }
    return full;
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

    // Header label — derived from the API's own reverse-geocoded location,
    // not user-entered. Only overwrite the previous value if this response
    // actually has a city; a malformed/partial one shouldn't blank it out.
    JsonObject loc = doc["location"].as<JsonObject>();
    String city = loc["city"] | "";
    if (city.length() > 0) {
        String prov = loc["state_prov"] | "";
        data.observerName = prov.length() > 0 ? (city + ", " + abbreviateRegion(prov)) : city;
    }

    Serial.printf("[AstroAPI] Parsed OK — %s  illum=%.1f%%  sun=%.1f°  loc=%s\n",
                  data.moonPhase.c_str(), data.moonIllumination, data.sunAltitude,
                  data.observerName.c_str());
    return true;
}

// ─── Public: begin ────────────────────────────────────────────────────────────
// Loads a persisted observer-location override, if the web dashboard has ever
// saved one. Falls back silently to the OBSERVER_LAT/LON compiled from
// secrets.h when no override file exists (first boot, or a fresh LittleFS).

void AstroAPI::begin() {
    if (!LittleFS.exists(LOCATION_PATH)) return;

    File f = LittleFS.open(LOCATION_PATH, "r");
    if (!f) return;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, f);
    f.close();
    if (err) {
        Serial.printf("[AstroAPI] Location override parse error: %s\n", err.c_str());
        return;
    }

    float lat = doc["lat"] | OBSERVER_LAT;
    float lon = doc["lon"] | OBSERVER_LON;
    if (lat >= -90.0f && lat <= 90.0f && lon >= -180.0f && lon <= 180.0f) {
        lat_ = lat;
        lon_ = lon;
        Serial.printf("[AstroAPI] Using saved location override: %.6f, %.6f\n", lat_, lon_);
    }
}

// ─── Public: setLocation ──────────────────────────────────────────────────────
// The header label (data.observerName) isn't set here — it's re-derived from
// the API's own location block on the fetch() that follows a location change.

bool AstroAPI::setLocation(float lat, float lon) {
    if (lat < -90.0f || lat > 90.0f || lon < -180.0f || lon > 180.0f)
        return false;

    lat_ = lat;
    lon_ = lon;

    JsonDocument doc;
    doc["lat"] = lat_;
    doc["lon"] = lon_;
    File f = LittleFS.open(LOCATION_PATH, "w");
    if (!f) {
        Serial.println("[AstroAPI] Location override write failed");
        return false;
    }
    serializeJson(doc, f);
    f.close();
    return true;
}

// ─── Public: fetch ────────────────────────────────────────────────────────────

bool AstroAPI::fetch() {
    String url = "https://api.ipgeolocation.io/v2/astronomy"
                 "?apiKey=" + String(ASTRO_API_KEY) +
                 "&lat="    + String(lat_, 6) +
                 "&long="   + String(lon_, 6) +
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
