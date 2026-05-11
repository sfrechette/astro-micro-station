#pragma once
// Copy this file to secrets.h and fill in your values.
// secrets.h is gitignored — never commit it.

// ── WiFi (required) ───────────────────────────────────────────────────────────
#define WIFI_SSID       "YOUR_SSID"
#define WIFI_PASSWORD   "YOUR_PASSWORD"

// ── ipgeolocation.io Astronomy API (required) ────────────────────────────────
// Free tier: 1000 req/day — sign up at https://app.ipgeolocation.io/signup
#define ASTRO_API_KEY   "YOUR_IPGEOLOCATION_API_KEY"

// ── Observer location (required) ─────────────────────────────────────────────
#define OBSERVER_LAT    0.0f               // decimal degrees, positive = North
#define OBSERVER_LON    0.0f               // decimal degrees, negative = West
#define OBSERVER_TZ     "America/Toronto"
#define UTC_OFFSET_SEC  -18000             // EDT = -14400  |  EST = -18000

// ── MQTT broker credentials (optional — only needed if MQTT_ENABLED in config.h)
#define MQTT_BROKER     "192.168.1.x"
#define MQTT_USER       ""                 // leave empty if broker has no auth
#define MQTT_PASS       ""
