#pragma once
#include <Arduino.h>
#include "config.h"
#include "AstroAPI.h"

// ═══════════════════════════════════════════════════════════════════════════════
//  MQTTManager.h  –  HA status publisher declarations
//  Implementation: src/MQTTManager.cpp
//
//  Stub class compiled when MQTT_ENABLED is not defined.
// ═══════════════════════════════════════════════════════════════════════════════

#ifdef MQTT_ENABLED
#include <PubSubClient.h>
#include <WiFiClient.h>

class MQTTManager {
public:
    MQTTManager();

    void begin();
    void loop();
    void publishStatus(const char* msg);
    void publishAstroData(const AstroData& data);

    bool isConnected() const { return _connected; }

private:
    WiFiClient   _wifiClient;
    PubSubClient _mqtt;
    bool         _connected = false;

    bool reconnect();
    void publishDiscovery();
};

#else

// ─── Stub: MQTT disabled ──────────────────────────────────────────────────────
class MQTTManager {
public:
    void begin() {}
    void loop() {}
    void publishStatus(const char*) {}
    void publishAstroData(const AstroData&) {}
    bool isConnected() const { return false; }
};

#endif  // MQTT_ENABLED
