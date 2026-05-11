#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
//  config.h  –  Astro Micro Station · T-Display S3 Pro
//  Non-sensitive settings — safe to commit. Credentials live in secrets.h (gitignored).
// ═══════════════════════════════════════════════════════════════════════════════

#include "secrets.h"

// ── MQTT (optional — comment out MQTT_ENABLED to disable) ────────────────────
#define MQTT_ENABLED
#define MQTT_PORT       1883
#define MQTT_CLIENT_ID  "astro-micro-station"
#define MQTT_TOPIC_STATUS "astronomy"

// ── Brightness buttons (active LOW, internal pull-up) ─────────────────────────
#define BTN_BRIGHT_UP_PIN   16   // IO16 — increase brightness
#define BTN_BRIGHT_DN_PIN   12   // IO12 — decrease brightness
#define BACKLIGHT_MIN        10   // minimum visible level (0 = off)
#define BACKLIGHT_MAX       255   // maximum level
#define BACKLIGHT_STEP       15   // change per press / repeat tick

// ── Display behaviour ─────────────────────────────────────────────────────────
#define BACKLIGHT_NORMAL     180            // 0-255
#define BACKLIGHT_DIM        40
#define BACKLIGHT_RED_MODE   60
#define DIM_TIMEOUT_SEC      30             // dim after this many seconds idle
#define SLEEP_TIMEOUT_SEC    120            // deep sleep after this (0 = never)

// ── Refresh intervals ─────────────────────────────────────────────────────────
#define API_REFRESH_INTERVAL_MS   900000UL  // re-fetch astronomy data every 15 min
#define NTP_SYNC_INTERVAL_MS      3600000UL // re-sync NTP every 1h
#define PROXIMITY_POLL_MS         200       // LTR553 proximity poll rate

// ── Proximity wake threshold (LTR-553) ───────────────────────────────────────
#define PROX_WAKE_THRESHOLD  50             // ADC counts; tune for your hand distance
