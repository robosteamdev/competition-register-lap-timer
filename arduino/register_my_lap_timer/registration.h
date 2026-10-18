#pragma once

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <time.h>

static const char FIRMWARE_VERSION[] = "register-laptimer/1.0.0";
// Match the real lap timer: D1/GPIO5, LOW = blocked, stable for 30 ms.
static const uint8_t SENSOR_PIN = D1;
static const bool SENSOR_ACTIVE_LOW = true;
static const uint32_t DEBOUNCE_MS = 30;
static const uint32_t HEARTBEAT_MS = 30000UL;
static const uint32_t WIFI_TIMEOUT_MS = 20000UL;
static const uint32_t RECONNECT_WAIT_MS = 5000UL;
static const uint32_t LED_BLINK_MS = 120;

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);
char deviceId[18];
char topicStatus[72];
bool credentialsOK = false;
bool wifiAttempt = false;
bool wifiWasConnected = false;
bool mqttAttempted = false;
bool instructionsPrinted = false;
bool heartbeatPending = false;
uint32_t wifiStartedMs = 0;
uint32_t wifiRetryMs = 0;
uint32_t lastDotMs = 0;
uint32_t lastReconnectMs = 0;
uint32_t lastHeartbeatMs = 0;
uint32_t lastPublishAttemptMs = 0;
int lastSensorRaw = HIGH;
int sensorDebounced = HIGH;
uint32_t lastChangeMs = 0;
bool lastObstacle = false;
uint32_t crossingCount = 0;
uint32_t lastCrossingMs = 0;
bool ledBlinking = false;
uint32_t ledStartedMs = 0;

bool needsEditing(const char *value) {
    return !value[0] || strstr(value, "CHANGE_ME") != nullptr ||
           strncmp(value, "your_", 5) == 0 || strncmp(value, "your-", 5) == 0;
}

void checkSetting(const char *name, const char *value) {
    if (!needsEditing(value)) return;
    Serial.print(F("Edit credentials.h: #define "));
    Serial.println(name);
    credentialsOK = false;
}

void validateCredentials() {
    credentialsOK = true;
    checkSetting("WIFI_SSID", WIFI_SSID);
    checkSetting("WIFI_PASSWORD", WIFI_PASSWORD);
    checkSetting("MQTT_BROKER", MQTT_BROKER);
    checkSetting("MQTT_USER", MQTT_USER);
    checkSetting("MQTT_PASSWORD", MQTT_PASSWORD);
    if (MQTT_PORT < 1 || MQTT_PORT > 65535) {
        Serial.println(F("Edit credentials.h: #define MQTT_PORT (1-65535)"));
        credentialsOK = false;
    }
    if (!credentialsOK) {
        Serial.println(F("Stopped. Edit settings, upload again. LED blinks fast."));
    }
}

bool isObstacle(int value) {
    return SENSOR_ACTIVE_LOW ? value == LOW : value == HIGH;
}

void updateSensor() {
    const uint32_t now = millis();
    const int raw = digitalRead(SENSOR_PIN);
    if (raw != lastSensorRaw) {
        lastSensorRaw = raw;
        lastChangeMs = now;
    }
    if (now - lastChangeMs >= DEBOUNCE_MS) sensorDebounced = raw;
    const bool obstacleNow = isObstacle(sensorDebounced);
    if (obstacleNow && !lastObstacle) {
        ++crossingCount;
        Serial.print(F("BEAM #"));
        Serial.print(crossingCount);
        Serial.print(F(" crossed  ("));
        if (crossingCount == 1) {
            Serial.print(F("first crossing"));
        } else {
            Serial.print('+');
            Serial.print((now - lastCrossingMs) / 1000.0, 2);
            Serial.print(F(" s since the last crossing"));
        }
        Serial.println(')');
        lastCrossingMs = now;
        digitalWrite(LED_BUILTIN, LOW);
        ledStartedMs = now;
        ledBlinking = true;
    }
    lastObstacle = obstacleNow;
    if (ledBlinking && now - ledStartedMs >= LED_BLINK_MS) {
        digitalWrite(LED_BUILTIN, HIGH);
        ledBlinking = false;
    }
}

void printNextSteps() {
    if (instructionsPrinted) return;
    instructionsPrinted = true;
    Serial.println(F("--- Next ---"));
    Serial.println(F("Website: https://competitions.robosteam.eu/"));
    Serial.println(F("Sign in with a Judge or Organiser account."));
    Serial.println(F("Judge: Select Category -> Add my lap timer"));
    Serial.println(F("or Organiser: Devices -> Add my lap timer"));
    Serial.print(F("Claim lap timer by MAC: "));
    Serial.println(deviceId);
    Serial.println(F("Test the sensor, then flash competition timer firmware:"));
    Serial.println(F("https://github.com/robosteamdev/robosteam-lap-timer"));
    Serial.println(F("This sketch sends no lap events or competition commands."));
}

bool publishHeartbeat() {
    // Same keys as the real firmware. No NTP library needed: ESP8266 has SNTP.
    const time_t epoch = time(nullptr);
    const bool ntpSynced = epoch >= 1704067200; // 2024-01-01 UTC
    time_t timestamp = ntpSynced ? epoch : static_cast<time_t>(millis() / 1000);
    struct tm utc;
    gmtime_r(&timestamp, &utc);
    char ts[32];
    strftime(ts, sizeof(ts), "%Y-%m-%dT%H:%M:%S.000Z", &utc);
    char payload[256];
    const int length = snprintf(payload, sizeof(payload),
        "{\"device_id\":\"%s\",\"ts\":\"%s\",\"firmware_version\":\"%s\","
        "\"uptime_ms\":%lu,\"run_active\":false,\"ntp_synced\":%s,\"seq\":0}",
        deviceId, ts, FIRMWARE_VERSION, static_cast<unsigned long>(millis()),
        ntpSynced ? "true" : "false");
    if (length < 0 || static_cast<size_t>(length) >= sizeof(payload) ||
        !mqtt.publish(topicStatus, payload, false)) {
        Serial.println(F("Heartbeat send failed; retrying in 5 s."));
        return false;
    }
    Serial.println(F("Heartbeat sent (every 30 s)."));
    // Publish success reports a connection write; check registration on the website.
    if (!instructionsPrinted) {
        Serial.print(F("Heartbeat written to broker connection (MAC "));
        Serial.print(deviceId);
        Serial.println(')');
        Serial.println(F("Confirm ONLINE on the website; claiming approves the timer."));
        printNextSteps();
    }
    return true;
}

void printMqttFailure(int state) {
    Serial.print(F("MQTT failed (code "));
    Serial.print(state);
    Serial.print(F("): "));
    switch (state) {
        case -4: Serial.println(F("server timeout. Check broker and network.")); break;
        case -3: Serial.println(F("connection lost. Check Wi-Fi and broker.")); break;
        case -2: Serial.println(F("broker unreachable. Check host, TCP port, firewall.")); break;
        case -1: Serial.println(F("disconnected. Check network and broker.")); break;
        case 1: Serial.println(F("protocol refused. Use MQTT 3.1.1 TCP.")); break;
        case 2: Serial.println(F("client ID refused. Ask the administrator.")); break;
        case 3: Serial.println(F("broker unavailable. Ask the administrator.")); break;
        case 4: Serial.println(F("bad credentials. Edit MQTT_USER / MQTT_PASSWORD.")); break;
        case 5: Serial.println(F("not authorised. Check MQTT account permissions.")); break;
        default: Serial.println(F("unknown error. Ask the administrator.")); break;
    }
    Serial.println(F("Retrying in 5 s. Sensor test stays enabled."));
}

void startWifi() {
    Serial.print(F("Wi-Fi: connecting"));
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    wifiAttempt = true;
    wifiStartedMs = lastDotMs = millis();
}

void maintainWifi() {
    const uint32_t now = millis();
    if (WiFi.status() == WL_CONNECTED) {
        if (!wifiWasConnected) {
            Serial.println();
            Serial.print(F("Wi-Fi OK. IP: "));
            Serial.println(WiFi.localIP());
            configTime(0, 0, "pool.ntp.org", "time.google.com");
            wifiWasConnected = true;
            wifiAttempt = false;
            mqttAttempted = false;
        }
        return;
    }
    if (wifiWasConnected) {
        Serial.println(F("Wi-Fi lost. Reconnecting; sensor test stays enabled."));
        mqtt.disconnect();
        wifiWasConnected = false;
        wifiAttempt = false;
        wifiRetryMs = now;
    }
    if (wifiAttempt) {
        if (now - lastDotMs >= 500) {
            Serial.print('.');
            lastDotMs = now;
        }
        if (now - wifiStartedMs >= WIFI_TIMEOUT_MS) {
            Serial.println();
            if (WiFi.status() == WL_NO_SSID_AVAIL) {
                Serial.println(F("Network not found. Check Wi-Fi name and range."));
            } else if (WiFi.status() == WL_CONNECT_FAILED) {
                Serial.println(F("Wi-Fi login failed. Check the password."));
            } else {
                Serial.println(F("Wi-Fi timed out. Check name, password and signal."));
            }
            Serial.println(F("ESP8266 needs 2.4 GHz Wi-Fi, not a 5 GHz-only network."));
            Serial.println(F("Use a network without a web login. Retrying in 5 s."));
            WiFi.disconnect();
            wifiAttempt = false;
            wifiRetryMs = now;
        }
    } else if (now - wifiRetryMs >= RECONNECT_WAIT_MS) {
        startWifi();
    }
}

void maintainMqtt() {
    if (WiFi.status() != WL_CONNECTED) return;
    uint32_t now = millis();
    if (!mqtt.connected()) {
        if (mqttAttempted && now - lastReconnectMs < RECONNECT_WAIT_MS) return;
        mqttAttempted = true;
        Serial.println(F("MQTT: connecting with MAC as client ID..."));
        const bool connected = mqtt.connect(deviceId, MQTT_USER, MQTT_PASSWORD);
        lastReconnectMs = millis();
        if (!connected) {
            printMqttFailure(mqtt.state());
            return;
        }
        heartbeatPending = true;
    }
    mqtt.loop();
    now = millis();
    if ((heartbeatPending || now - lastHeartbeatMs >= HEARTBEAT_MS) &&
        (lastPublishAttemptMs == 0 || now - lastPublishAttemptMs >= RECONNECT_WAIT_MS)) {
        lastPublishAttemptMs = now;
        if (publishHeartbeat()) {
            lastHeartbeatMs = millis();
            heartbeatPending = false;
        }
    }
}

void setup() {
    Serial.begin(115200);
    delay(200);
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH); // NodeMCU onboard LED is active-low.
    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(false);
    uint8_t mac[6];
    WiFi.macAddress(mac);
    snprintf(deviceId, sizeof(deviceId), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    Serial.println(F("\n=== RoboSTEAM Register My Lap Timer ==="));
    Serial.print(F("Firmware: "));
    Serial.println(FIRMWARE_VERSION);
    Serial.println(F("+--------------------------+"));
    Serial.print(F("| MAC: "));
    Serial.print(deviceId);
    Serial.println(F("   |"));
    Serial.println(F("+--------------------------+"));
    validateCredentials();
    if (!credentialsOK) return;
    pinMode(SENSOR_PIN, INPUT_PULLUP);
    sensorDebounced = lastSensorRaw = digitalRead(SENSOR_PIN);
    lastObstacle = isObstacle(sensorDebounced);
    lastChangeMs = millis();
    Serial.println(F("Sensor: D1/GPIO5, active-low, 30 ms debounce."));
    Serial.println(lastObstacle ? F("Sensor starts BLOCKED. Clear it before testing.")
                                : F("Sensor starts CLEAR. Cross the beam to test."));
    snprintf(topicStatus, sizeof(topicStatus), "robosteam/laptimer/%s/status", deviceId);
    mqtt.setServer(MQTT_BROKER, MQTT_PORT);
    mqtt.setSocketTimeout(1);
    if (!mqtt.setBufferSize(512)) {
        Serial.println(F("MQTT buffer allocation failed. Restart the device."));
        credentialsOK = false;
        return;
    }
    startWifi();
}

void loop() {
    if (!credentialsOK) {
        digitalWrite(LED_BUILTIN, (millis() / 100) % 2 ? HIGH : LOW);
        delay(1);
        return;
    }
    updateSensor();
    maintainWifi();
    maintainMqtt();
    updateSensor();
    delay(1);
}
