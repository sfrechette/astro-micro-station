#include "WebDashboard.h"
#include "config.h"
#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>
#include "dashboard_html.h"

// ═══════════════════════════════════════════════════════════════════════════════
//  WebDashboard.cpp  –  serves dashboard_html.h at "/", AstroData as JSON at
//  "/api/astro", and observer-location read/write at "/api/location".
//  AsyncWebServer handles requests off its own task — GET routes need no
//  loop() polling. The location POST only flips a flag; the actual re-fetch
//  (blocking HTTPS) runs from loop() on the main task, same as the periodic
//  refresh already does, so it never runs on the async server's task/stack.
// ═══════════════════════════════════════════════════════════════════════════════

static AsyncWebServer server(WEB_DASHBOARD_PORT);
static AstroAPI*      astroRef = nullptr;
static volatile bool  locationChanged = false;

static void addTwilight(JsonObject obj, const TwilightTimes& t) {
    obj["astronomicalBegin"] = t.astronomicalBegin;
    obj["astronomicalEnd"]   = t.astronomicalEnd;
    obj["nauticalBegin"]     = t.nauticalBegin;
    obj["nauticalEnd"]       = t.nauticalEnd;
    obj["civilBegin"]        = t.civilBegin;
    obj["civilEnd"]          = t.civilEnd;
    obj["blueHourBegin"]     = t.blueHourBegin;
    obj["blueHourEnd"]       = t.blueHourEnd;
    obj["goldenHourBegin"]   = t.goldenHourBegin;
    obj["goldenHourEnd"]     = t.goldenHourEnd;
}

static void handleApiAstro(AsyncWebServerRequest* request) {
    JsonDocument doc;
    const AstroData& d = astroRef->data;

    doc["valid"]        = d.valid;
    doc["fetchDate"]    = d.fetchDate;
    doc["currentTime"]  = d.currentTime;
    doc["locationName"] = d.observerName;

    JsonObject sun = doc["sun"].to<JsonObject>();
    sun["sunrise"]    = d.sunrise;
    sun["sunset"]     = d.sunset;
    sun["solarNoon"]  = d.solarNoon;
    sun["dayLength"]  = d.dayLength;
    sun["altitude"]   = d.sunAltitude;
    sun["azimuth"]    = d.sunAzimuth;
    sun["distanceKm"] = d.sunDistance;
    sun["midNight"]   = d.midNight;
    sun["nightBegin"] = d.nightBegin;
    sun["nightEnd"]   = d.nightEnd;
    addTwilight(sun["morning"].to<JsonObject>(), d.morning);
    addTwilight(sun["evening"].to<JsonObject>(), d.evening);

    JsonObject moon = doc["moon"].to<JsonObject>();
    moon["phase"]        = d.moonPhase;
    moon["phaseName"]    = moonPhaseName(d.moonPhase);
    moon["illumination"] = d.moonIllumination;
    moon["moonrise"]     = d.moonrise;
    moon["moonset"]      = d.moonset;
    moon["altitude"]     = d.moonAltitude;
    moon["azimuth"]      = d.moonAzimuth;
    moon["angle"]        = d.moonAngle;
    moon["distanceKm"]   = d.moonDistance;

    // Browser polls at this cadence — single source of truth, stays in sync
    // with how often astroAPI.fetch() actually refreshes on-device.
    doc["refreshIntervalMs"] = API_REFRESH_INTERVAL_MS;

    AsyncResponseStream* response = request->beginResponseStream("application/json");
    response->addHeader("Cache-Control", "no-store");
    serializeJson(doc, *response);
    request->send(response);
}

static void handleGetLocation(AsyncWebServerRequest* request) {
    JsonDocument doc;
    doc["lat"] = astroRef->observerLat();
    doc["lon"] = astroRef->observerLon();
    AsyncResponseStream* response = request->beginResponseStream("application/json");
    serializeJson(doc, *response);
    request->send(response);
}

static void handleUseHomeLocation(AsyncWebServerRequest* request) {
    if (!astroRef->useHomeLocation()) {
        request->send(500, "application/json", "{\"error\":\"failed to save home location\"}");
        return;
    }
    Serial.println("[WebDashboard] Reset to home location");
    locationChanged = true;

    JsonDocument resp;
    resp["ok"]  = true;
    resp["lat"] = astroRef->observerLat();
    resp["lon"] = astroRef->observerLon();
    String out;
    serializeJson(resp, out);
    request->send(200, "application/json", out);
}

void webDashboardBegin(AstroAPI* astro) {
    astroRef = astro;

    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(200, "text/html", DASHBOARD_HTML);
    });

    server.on("/api/astro", HTTP_GET, handleApiAstro);
    server.on("/api/location", HTTP_GET, handleGetLocation);
    server.on("/api/location/home", HTTP_POST, handleUseHomeLocation);

    AsyncCallbackJsonWebHandler* locationHandler = new AsyncCallbackJsonWebHandler(
        "/api/location",
        [](AsyncWebServerRequest* request, JsonVariant& json) {
            JsonObject body = json.as<JsonObject>();
            if (body["lat"].isNull() || body["lon"].isNull()) {
                request->send(400, "application/json", "{\"error\":\"lat and lon are required\"}");
                return;
            }
            float lat = body["lat"].as<float>();
            float lon = body["lon"].as<float>();
            if (!astroRef->setLocation(lat, lon)) {
                request->send(400, "application/json", "{\"error\":\"lat must be -90..90, lon -180..180\"}");
                return;
            }
            Serial.printf("[WebDashboard] Observer location updated: %.6f, %.6f\n", lat, lon);
            locationChanged = true;

            JsonDocument resp;
            resp["ok"]  = true;
            resp["lat"] = lat;
            resp["lon"] = lon;
            String out;
            serializeJson(resp, out);
            request->send(200, "application/json", out);
        });
    server.addHandler(locationHandler);

    server.onNotFound([](AsyncWebServerRequest* request) {
        request->send(404, "text/plain", "Not found");
    });

    server.begin();
    Serial.println("[WebDashboard] Server started");
}

bool webDashboardConsumeLocationChange() {
    if (!locationChanged) return false;
    locationChanged = false;
    return true;
}
