#include "WebDashboard.h"
#include "config.h"
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "dashboard_html.h"

// ═══════════════════════════════════════════════════════════════════════════════
//  WebDashboard.cpp  –  serves dashboard_html.h at "/" and AstroData as JSON
//  at "/api/astro". AsyncWebServer handles requests off its own callbacks —
//  nothing to poll from loop(), so main.cpp's render/touch loop is untouched.
// ═══════════════════════════════════════════════════════════════════════════════

static AsyncWebServer server(WEB_DASHBOARD_PORT);
static AstroAPI*      astroRef = nullptr;

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

    doc["valid"]       = d.valid;
    doc["fetchDate"]   = d.fetchDate;
    doc["currentTime"] = d.currentTime;

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

void webDashboardBegin(AstroAPI* astro) {
    astroRef = astro;

    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send_P(200, "text/html", DASHBOARD_HTML);
    });

    server.on("/api/astro", HTTP_GET, handleApiAstro);

    server.onNotFound([](AsyncWebServerRequest* request) {
        request->send(404, "text/plain", "Not found");
    });

    server.begin();
    Serial.println("[WebDashboard] Server started");
}
