#include <Arduino.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <time.h>

#include "config.h"
#include "AstroAPI.h"
#include <LittleFS.h>
#include "DisplayManager.h"
#include "MQTTManager.h"
#include "WebDashboard.h"

// ─── LTR-553 proximity sensor ────────────────────────────────────────────────
#include <LightSensorDrv.hpp>
static SensorLTR553 ltr553;
static bool         ltr553_ok = false;

// ─── CST226SE capacitive touch (I2C, shared bus with LTR553) ─────────────────
#include <touch/TouchDrvCST226.hpp>
static TouchDrvCST226 touch;
static bool           touch_ok = false;

// ═══════════════════════════════════════════════════════════════════════════════
//  Globals
// ═══════════════════════════════════════════════════════════════════════════════

static DisplayManager display;
static AstroAPI       astroAPI;
static MQTTManager      mqtt;

static WiFiUDP    ntpUDP;
static NTPClient  ntp(ntpUDP, "pool.ntp.org", 0, 3600000);

// ─── App state ────────────────────────────────────────────────────────────────
static int      currentScreen   = 0;   // 0=Moon  1=Day  2=Morning  3=Evening
static uint32_t lastApiRefresh  = 0;
static uint32_t lastNtpSync     = 0;
static uint32_t lastProxPoll    = 0;
static uint32_t lastInteraction = 0;

// ─── Brightness state ─────────────────────────────────────────────────────────
static uint8_t  userBrightness  = BACKLIGHT_NORMAL;  // survives dim/wake cycles
static uint32_t brightRepeatMs  = 0;                 // next repeat-fire timestamp

// ─── Render dirty tracking ────────────────────────────────────────────────────
static int      lastScreen      = -1;    // force draw on first loop
static bool     lastRedMode     = false;
static uint32_t lastRenderFetch = 0;
static bool     wasDimmed       = false;

// ─── Touch state ──────────────────────────────────────────────────────────────
static uint16_t touchX           = 0, touchY = 0;
static bool     inGesture        = false;  // true from first touch → confirmed release
static uint32_t gestureStartMs   = 0;      // when finger first went down (never reset by glitches)
static uint32_t liftDetectedMs   = 0;      // first no-touch frame after a gesture
static bool     gestureLongFired = false;  // long-press already handled this gesture
static bool     touchIgnore      = false;  // suppress intent for wake-from-dim touch
static uint8_t  noTouchCount     = 0;      // consecutive no-touch frames

// ═══════════════════════════════════════════════════════════════════════════════
//  Helpers
// ═══════════════════════════════════════════════════════════════════════════════

static void connectWiFi() {
    Serial.printf("[WiFi] Connecting to '%s' ...\n", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    uint32_t t = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t < 15000) {
        delay(500);
        Serial.printf("[WiFi] status=%d  elapsed=%lums\n",
                      WiFi.status(), (unsigned long)(millis() - t));
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("[WiFi] Connected!  IP=%s  RSSI=%ddBm\n",
                      WiFi.localIP().toString().c_str(), WiFi.RSSI());
    } else {
        Serial.printf("[WiFi] Failed (status=%d) — offline mode\n", WiFi.status());
    }
}

static void showSplash() {
    display.tft.fillScreen(0x0841);
    display.tft.setTextDatum(TC_DATUM);
    display.tft.setTextColor(Pal::DKGREEN);
    display.tft.drawString("ASTRONOMY MICRO STATION", 240, 75, 4);
    display.tft.setTextColor(0x8410);
    display.tft.drawString("Connecting...", 240, 125, 2);
    display.tft.setTextDatum(TL_DATUM);
}

static void fetchAstro() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[fetchAstro] WiFi not connected — trying cache");
        if (!astroAPI.loadCache())
            Serial.println("[fetchAstro] No cache available — waiting for connectivity");
        return;
    }
    Serial.printf("[fetchAstro] Starting  RSSI=%ddBm  heap=%lu\n",
                  WiFi.RSSI(), (unsigned long)ESP.getFreeHeap());
    display.tft.setTextDatum(TC_DATUM);
    display.tft.setTextColor(Pal::DKGREEN);
    display.tft.drawString("Fetching astronomy data...", 240, 152, 1);
    display.tft.setTextDatum(TL_DATUM);

    bool ok = astroAPI.fetch();
    Serial.printf("[fetchAstro] %s  heap=%lu  min=%lu  maxBlock=%lu  psram=%lu\n",
                  ok ? "OK" : "FAILED",
                  (unsigned long)ESP.getFreeHeap(),
                  (unsigned long)ESP.getMinFreeHeap(),
                  (unsigned long)ESP.getMaxAllocHeap(),
                  (unsigned long)ESP.getFreePsram());
    if (ok) mqtt.publishAstroData(astroAPI.data);
    lastApiRefresh = millis();
}

// ═══════════════════════════════════════════════════════════════════════════════
//  setup
// ═══════════════════════════════════════════════════════════════════════════════

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n[Boot] Astro Micro Station  (astro-only debug mode)");

    display.begin();
    showSplash();

    if (!LittleFS.begin(true))    // true = format partition on first boot if not yet formatted
        Serial.println("[LittleFS] Mount failed — offline cache unavailable");
    else
        Serial.println("[LittleFS] OK");

    Wire.begin(LTR553_SDA, LTR553_SCL);

    if (ltr553.begin(Wire, LTR553_SLAVE_ADDRESS, LTR553_SDA, LTR553_SCL)) {
        ltr553.enableProximity();
        ltr553.enableLightSensor();
        ltr553_ok = true;
        Serial.println("[LTR553] OK");
    } else {
        Serial.println("[LTR553] Not found");
    }

    touch.setPins(TOUCH_RST, TOUCH_IRQ);
    if (touch.begin(Wire, CST226SE_SLAVE_ADDRESS, TOUCH_SDA, TOUCH_SCL)) {
        touch_ok = true;
        Serial.println("[Touch] CST226SE OK");
    } else {
        Serial.println("[Touch] CST226SE not found");
    }

    connectWiFi();

    if (WiFi.status() == WL_CONNECTED) {
        delay(500);   // give DNS resolver time to become ready after WiFi connect
        ntp.begin();
        ntp.update();
        Serial.printf("[NTP] %s\n", ntp.getFormattedTime().c_str());
    }

    mqtt.begin();
    fetchAstro();

    if (WiFi.status() == WL_CONNECTED) {
        webDashboardBegin(&astroAPI);
        Serial.printf("[WebDashboard] http://%s/\n", WiFi.localIP().toString().c_str());
    }

    pinMode(BTN_BRIGHT_UP_PIN, INPUT_PULLUP);
    pinMode(BTN_BRIGHT_DN_PIN, INPUT_PULLUP);

    lastInteraction = millis();
    Serial.println("[Boot] Ready");
}

// ═══════════════════════════════════════════════════════════════════════════════
//  loop
// ═══════════════════════════════════════════════════════════════════════════════

void loop() {
    const uint32_t now_ms = millis();
    const time_t   now    = ntp.getEpochTime();

    // ── Periodic tasks ────────────────────────────────────────────────────────
    mqtt.loop();

    if (now_ms - lastNtpSync > NTP_SYNC_INTERVAL_MS) {
        ntp.forceUpdate();
        lastNtpSync = now_ms;
        Serial.printf("[Mem]  heap=%lu  min=%lu  maxBlock=%lu  psram=%lu\n",
                      (unsigned long)ESP.getFreeHeap(),
                      (unsigned long)ESP.getMinFreeHeap(),
                      (unsigned long)ESP.getMaxAllocHeap(),
                      (unsigned long)ESP.getFreePsram());
    }

    if (now_ms - lastApiRefresh > API_REFRESH_INTERVAL_MS) {
        if (WiFi.status() == WL_CONNECTED) {
            bool ok = astroAPI.fetch();
            if (ok) mqtt.publishAstroData(astroAPI.data);
        }
        lastApiRefresh = now_ms;
    }

    // ── Proximity wake ────────────────────────────────────────────────────────
    if (ltr553_ok && now_ms - lastProxPoll > PROXIMITY_POLL_MS) {
        if (ltr553.getProximity() > PROX_WAKE_THRESHOLD && display.dimmed) {
            display.setBacklight(userBrightness);
            display.dimmed  = false;
            lastInteraction = now_ms;
        }
        lastProxPoll = now_ms;
    }

    // ── Backlight dim ─────────────────────────────────────────────────────────
    if (!display.dimmed && now_ms - lastInteraction > (DIM_TIMEOUT_SEC * 1000UL)) {
        display.setBacklight(BACKLIGHT_DIM);
        display.dimmed = true;
    }

    // ── Touch ─────────────────────────────────────────────────────────────────
    // CST226SE reports portrait coords (x:0..221, y:0..479); remap to landscape.
    bool nowTouched = false;
    if (touch_ok && touch.isPressed()) {
        int16_t tx, ty;
        if (touch.getPoint(&tx, &ty, 1) > 0) {
            touchX = (uint16_t)ty;
            touchY = (uint16_t)(221 - tx);
        }
        nowTouched = true;
    }

    // Count consecutive no-touch frames; record the first one for accurate lift timing
    if (nowTouched) {
        noTouchCount = 0;
    } else {
        if (noTouchCount == 0) liftDetectedMs = now_ms;  // first frame after lift
        if (noTouchCount < 255) noTouchCount++;
    }

    // Rising edge — start a new gesture (inGesture stays true through brief glitches)
    if (nowTouched && !inGesture) {
        inGesture        = true;
        gestureStartMs   = now_ms;
        gestureLongFired = false;
        if (display.dimmed) {
            display.setBacklight(userBrightness);
            display.dimmed  = false;
            lastInteraction = now_ms;
            touchIgnore     = true;   // this touch is just a wake — ignore its intent
        }
    }

    // Long press: 800 ms elapsed and finger still present (< 3 no-touch frames)
    if (inGesture && noTouchCount < 3 && !touchIgnore && !gestureLongFired
            && (now_ms - gestureStartMs > 800)) {
        display.setRedMode(!display.redMode);
        lastInteraction  = now_ms;
        gestureLongFired = true;
    }

    // Confirmed release: 5 consecutive no-touch frames (~250 ms) — end the gesture
    if (inGesture && noTouchCount >= 5) {
        uint32_t held = liftDetectedMs - gestureStartMs;  // true hold duration
        if (!touchIgnore && !gestureLongFired && held < 700) {
            currentScreen   = (currentScreen + 1) % 4;
            lastInteraction = now_ms;
            Serial.printf("[Nav] Screen %d\n", currentScreen);
        }
        inGesture   = false;
        touchIgnore = false;
    }

    // ── Brightness buttons (IO16 up, IO12 down) ───────────────────────────────
    if (!display.dimmed) {
        bool upHeld = (digitalRead(BTN_BRIGHT_UP_PIN) == LOW);
        bool dnHeld = (digitalRead(BTN_BRIGHT_DN_PIN) == LOW);
        if ((upHeld || dnHeld) && now_ms >= brightRepeatMs) {
            int delta = upHeld ? BACKLIGHT_STEP : -BACKLIGHT_STEP;
            userBrightness = (uint8_t)constrain((int)userBrightness + delta,
                                                BACKLIGHT_MIN, BACKLIGHT_MAX);
            display.setBacklight(userBrightness);
            lastInteraction = now_ms;
            brightRepeatMs  = now_ms + 200;   // 200 ms repeat rate while held
        }
    }

    // ── Render (dirty flag — only redraw when something actually changed) ────────
    bool needsRedraw = (currentScreen  != lastScreen)
                    || (display.redMode != lastRedMode)
                    || (lastApiRefresh  != lastRenderFetch)
                    || (wasDimmed && !display.dimmed);  // just woke up

    if (needsRedraw && !display.dimmed) {
        display.drawScreen(currentScreen, astroAPI.data, now);
        lastScreen      = currentScreen;
        lastRedMode     = display.redMode;
        lastRenderFetch = lastApiRefresh;
    }
    wasDimmed = display.dimmed;

    delay(50);
}
