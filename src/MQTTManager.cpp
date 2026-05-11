#include "MQTTManager.h"

// ═══════════════════════════════════════════════════════════════════════════════
//  MQTTManager.cpp  –  PubSubClient wrapper for Home Assistant alerts
//  Only compiled when MQTT_ENABLED is defined in config.h
// ═══════════════════════════════════════════════════════════════════════════════

#ifdef MQTT_ENABLED

MQTTManager::MQTTManager() : _mqtt(_wifiClient) {}

// ─── begin ────────────────────────────────────────────────────────────────────

void MQTTManager::begin() {
    _mqtt.setServer(MQTT_BROKER, MQTT_PORT);
    reconnect();
}

// ─── loop ─────────────────────────────────────────────────────────────────────

void MQTTManager::loop() {
    if (!_mqtt.connected()) reconnect();
    _mqtt.loop();
}

// ─── publishStatus ────────────────────────────────────────────────────────────

void MQTTManager::publishStatus(const char* msg) {
    if (!_connected) return;
    _mqtt.publish(MQTT_TOPIC_STATUS, msg, true);  // retained
}

// ─── publishAstroData ─────────────────────────────────────────────────────────

void MQTTManager::publishAstroData(const AstroData& d) {
    if (!_connected && !reconnect()) return;

    char buf[512];
    String base = String(MQTT_TOPIC_STATUS);

    // ── Sun ───────────────────────────────────────────────────────────────────
    snprintf(buf, sizeof(buf),
        "{\"date\":\"%s\",\"time\":\"%s\","
        "\"sunrise\":\"%s\",\"sunset\":\"%s\","
        "\"solar_noon\":\"%s\",\"day_length\":\"%s\","
        "\"altitude\":%.1f,\"azimuth\":%.1f,"
        "\"distance_Mkm\":%.1f,"
        "\"night_begin\":\"%s\",\"night_end\":\"%s\",\"mid_night\":\"%s\"}",
        d.fetchDate.c_str(), d.currentTime.c_str(),
        d.sunrise.c_str(), d.sunset.c_str(),
        d.solarNoon.c_str(), d.dayLength.c_str(),
        d.sunAltitude, d.sunAzimuth,
        d.sunDistance / 1e6f,
        d.nightBegin.c_str(), d.nightEnd.c_str(), d.midNight.c_str()
    );
    _mqtt.publish((base + "/sun").c_str(), buf, true);
    Serial.printf("[MQTT] → %s/sun (%d bytes)\n", MQTT_TOPIC_STATUS, strlen(buf));

    // ── Moon ──────────────────────────────────────────────────────────────────
    snprintf(buf, sizeof(buf),
        "{\"phase\":\"%s\",\"phase_name\":\"%s\","
        "\"illumination\":%.1f,"
        "\"moonrise\":\"%s\",\"moonset\":\"%s\","
        "\"altitude\":%.1f,\"azimuth\":%.1f,"
        "\"angle\":%.1f,\"distance_km\":%.0f}",
        d.moonPhase.c_str(), moonPhaseName(d.moonPhase).c_str(),
        d.moonIllumination,
        d.moonrise.c_str(), d.moonset.c_str(),
        d.moonAltitude, d.moonAzimuth,
        d.moonAngle, d.moonDistance
    );
    _mqtt.publish((base + "/moon").c_str(), buf, true);
    Serial.printf("[MQTT] → %s/moon (%d bytes)\n", MQTT_TOPIC_STATUS, strlen(buf));

    // ── Morning twilight ──────────────────────────────────────────────────────
    snprintf(buf, sizeof(buf),
        "{\"astro_begin\":\"%s\",\"astro_end\":\"%s\","
        "\"nautical_begin\":\"%s\",\"nautical_end\":\"%s\","
        "\"civil_begin\":\"%s\",\"civil_end\":\"%s\","
        "\"blue_begin\":\"%s\",\"blue_end\":\"%s\","
        "\"golden_begin\":\"%s\",\"golden_end\":\"%s\"}",
        d.morning.astronomicalBegin.c_str(), d.morning.astronomicalEnd.c_str(),
        d.morning.nauticalBegin.c_str(),     d.morning.nauticalEnd.c_str(),
        d.morning.civilBegin.c_str(),        d.morning.civilEnd.c_str(),
        d.morning.blueHourBegin.c_str(),     d.morning.blueHourEnd.c_str(),
        d.morning.goldenHourBegin.c_str(),   d.morning.goldenHourEnd.c_str()
    );
    _mqtt.publish((base + "/morning").c_str(), buf, true);
    Serial.printf("[MQTT] → %s/morning (%d bytes)\n", MQTT_TOPIC_STATUS, strlen(buf));

    // ── Evening twilight ──────────────────────────────────────────────────────
    snprintf(buf, sizeof(buf),
        "{\"golden_begin\":\"%s\",\"golden_end\":\"%s\","
        "\"blue_begin\":\"%s\",\"blue_end\":\"%s\","
        "\"civil_begin\":\"%s\",\"civil_end\":\"%s\","
        "\"nautical_begin\":\"%s\",\"nautical_end\":\"%s\","
        "\"astro_begin\":\"%s\",\"astro_end\":\"%s\"}",
        d.evening.goldenHourBegin.c_str(),   d.evening.goldenHourEnd.c_str(),
        d.evening.blueHourBegin.c_str(),     d.evening.blueHourEnd.c_str(),
        d.evening.civilBegin.c_str(),        d.evening.civilEnd.c_str(),
        d.evening.nauticalBegin.c_str(),     d.evening.nauticalEnd.c_str(),
        d.evening.astronomicalBegin.c_str(), d.evening.astronomicalEnd.c_str()
    );
    _mqtt.publish((base + "/evening").c_str(), buf, true);
    Serial.printf("[MQTT] → %s/evening (%d bytes)\n", MQTT_TOPIC_STATUS, strlen(buf));
}

// ─── publishDiscovery ─────────────────────────────────────────────────────────
// Publishes HA MQTT Discovery config for every sensor so they appear
// automatically as a single device — no configuration.yaml editing required.

void MQTTManager::publishDiscovery() {
    const char* dev =
        "\"device\":{"
        "\"identifiers\":[\"" MQTT_CLIENT_ID "\"],"
        "\"name\":\"Astro Micro Station\","
        "\"model\":\"T-Display S3 Pro\","
        "\"manufacturer\":\"LilyGO\"}";

    static const struct {
        const char* name;
        const char* uid;
        const char* topic;
        const char* tmpl;
        const char* unit;
        const char* icon;
    } S[] = {
        // Moon
        {"Moon Phase",        "moon_phase",       "astronomy/moon",    "{{ value_json.phase_name }}",    "",   "mdi:moon-waxing-crescent"},
        {"Moon Illumination", "moon_illumination", "astronomy/moon",    "{{ value_json.illumination }}",  "%",  "mdi:brightness-percent"  },
        {"Moonrise",          "moonrise",          "astronomy/moon",    "{{ value_json.moonrise }}",      "",   "mdi:moon-new"            },
        {"Moonset",           "moonset",           "astronomy/moon",    "{{ value_json.moonset }}",       "",   "mdi:moon-full"           },
        {"Moon Altitude",     "moon_altitude",     "astronomy/moon",    "{{ value_json.altitude }}",      "\xc2\xb0", "mdi:angle-acute"   },
        {"Moon Azimuth",      "moon_azimuth",      "astronomy/moon",    "{{ value_json.azimuth }}",       "\xc2\xb0", "mdi:compass-outline"},
        {"Moon Distance",     "moon_distance",     "astronomy/moon",    "{{ value_json.distance_km }}",   "km", "mdi:ruler"               },
        // Sun
        {"Sunrise",           "sunrise",           "astronomy/sun",     "{{ value_json.sunrise }}",       "",   "mdi:weather-sunset-up"   },
        {"Sunset",            "sunset",            "astronomy/sun",     "{{ value_json.sunset }}",        "",   "mdi:weather-sunset-down" },
        {"Solar Noon",        "solar_noon",        "astronomy/sun",     "{{ value_json.solar_noon }}",    "",   "mdi:white-balance-sunny" },
        {"Day Length",        "day_length",        "astronomy/sun",     "{{ value_json.day_length }}",    "",   "mdi:clock-outline"       },
        {"Sun Altitude",      "sun_altitude",      "astronomy/sun",     "{{ value_json.altitude }}",      "\xc2\xb0", "mdi:angle-acute"   },
        {"Sun Azimuth",       "sun_azimuth",       "astronomy/sun",     "{{ value_json.azimuth }}",       "\xc2\xb0", "mdi:compass-outline"},
        // Morning twilight
        {"Golden Hour AM",    "golden_hour_am",    "astronomy/morning", "{{ value_json.golden_begin }} - {{ value_json.golden_end }}", "", "mdi:weather-sunset"      },
        {"Blue Hour AM",      "blue_hour_am",      "astronomy/morning", "{{ value_json.blue_begin }} - {{ value_json.blue_end }}",     "", "mdi:weather-night"       },
        {"Civil Twilight AM", "civil_twilight_am", "astronomy/morning", "{{ value_json.civil_begin }} - {{ value_json.civil_end }}",   "", "mdi:weather-fog"         },
        // Evening twilight
        {"Golden Hour PM",    "golden_hour_pm",    "astronomy/evening", "{{ value_json.golden_begin }} - {{ value_json.golden_end }}", "", "mdi:weather-sunset"      },
        {"Blue Hour PM",      "blue_hour_pm",      "astronomy/evening", "{{ value_json.blue_begin }} - {{ value_json.blue_end }}",     "", "mdi:weather-night"       },
        {"Civil Twilight PM", "civil_twilight_pm", "astronomy/evening", "{{ value_json.civil_begin }} - {{ value_json.civil_end }}",   "", "mdi:weather-fog"         },
    };

    char dtopic[80];
    char buf[512];

    for (const auto& s : S) {
        snprintf(dtopic, sizeof(dtopic), "homeassistant/sensor/astro_%s/config", s.uid);
        if (s.unit[0]) {
            snprintf(buf, sizeof(buf),
                "{\"name\":\"%s\",\"unique_id\":\"astro_%s\","
                "\"state_topic\":\"%s\",\"value_template\":\"%s\","
                "\"unit_of_measurement\":\"%s\",\"icon\":\"%s\","
                "\"availability_topic\":\"" MQTT_TOPIC_STATUS "\",%s}",
                s.name, s.uid, s.topic, s.tmpl, s.unit, s.icon, dev);
        } else {
            snprintf(buf, sizeof(buf),
                "{\"name\":\"%s\",\"unique_id\":\"astro_%s\","
                "\"state_topic\":\"%s\",\"value_template\":\"%s\","
                "\"icon\":\"%s\","
                "\"availability_topic\":\"" MQTT_TOPIC_STATUS "\",%s}",
                s.name, s.uid, s.topic, s.tmpl, s.icon, dev);
        }
        _mqtt.publish(dtopic, buf, true);
        Serial.printf("[MQTT] Discovery: %s (%d bytes)\n", dtopic, strlen(buf));
    }
}

// ─── reconnect ────────────────────────────────────────────────────────────────

bool MQTTManager::reconnect() {
    if (_mqtt.connected()) { _connected = true; return true; }

    Serial.print("[MQTT] Connecting to " MQTT_BROKER "...");

    // Connect with LWT so HA marks device unavailable when it drops off
    bool ok = (strlen(MQTT_USER) > 0)
        ? _mqtt.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS,
                        MQTT_TOPIC_STATUS, 0, true, "offline")
        : _mqtt.connect(MQTT_CLIENT_ID, nullptr, nullptr,
                        MQTT_TOPIC_STATUS, 0, true, "offline");

    _connected = ok;

    if (ok) {
        Serial.println(" OK");
        publishStatus("online");
        publishDiscovery();
    } else {
        Serial.printf(" FAILED (rc=%d)\n", _mqtt.state());
    }

    return ok;
}

#endif  // MQTT_ENABLED
