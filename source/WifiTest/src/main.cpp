// Required for PlatformIO
#include <Arduino.h>

#include <esp_system.h>
#include <nvs_flash.h>
#include <Preferences.h>
#include <millisDelay.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <AsyncJson.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>
#include <WebSocketsClient.h>

// Local WiFi Settings
const char* softAP_ssid = "WifiTest";
const char* softAP_password = "12345678";

// External WiFi Settings
const char* ssid = "BenchRig";
const char* password = "12345678";

// WebSocket server settings
const char* websocket_host = "192.168.1.2";
const uint16_t websocket_port = 80;
const char* websocket_uri = "/ws";

// WiFi and WebSocket client objects
WebSocketsClient webSocket;
AsyncWebServer server(80);

// Delay objects for non-blocking reconnections
millisDelay ms_wifiReconnectDelay;
millisDelay ms_webSocketReconnectDelay;

// State tracking variables
bool b_wifiConnected = false;
bool b_webSocketConnected = false;

void setupSoftAP() {
    Serial.println("Starting SoftAP network...");
    WiFi.softAP(softAP_ssid, softAP_password);
    Serial.printf("SoftAP started. IP: %s\n", WiFi.softAPIP().toString().c_str());
}

bool connectToWiFi() {
    Serial.printf("Connecting to %s...\n", ssid);
    WiFi.begin(ssid, password);

    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
        if (millis() - startAttemptTime >= 500) {
            Serial.print(".");
            startAttemptTime = millis();
        }
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected to WiFi.");
        return true;
    } else {
        Serial.println("\nFailed to connect to WiFi. Retrying...");
        return false;
    }
}

void setupWebSocket() {
    Serial.println("Initializing WebSocket connection...");
    webSocket.begin(websocket_host, websocket_port, websocket_uri);
    webSocket.onEvent(webSocketEvent);
    ms_webSocketReconnectDelay.start(5000);  // Set delay for next reconnection attempt
    b_webSocketConnected = true;
}

void setupAsyncWebServer() {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "Hello from ESP32 Async Web Server!");
    });

    server.begin();
    Serial.println("Async Web Server started.");
}

void webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
        case WStype_DISCONNECTED:
            Serial.println("WebSocket disconnected.");
            b_webSocketConnected = false;
            ms_webSocketReconnectDelay.start(5000);
            break;

        case WStype_CONNECTED:
            Serial.printf("WebSocket connected to %s:%d%s\n", websocket_host, websocket_port, websocket_uri);
            b_webSocketConnected = true;
            webSocket.sendTXT("Hello, server!");
            break;

        case WStype_TEXT:
            Serial.printf("Received message: %s\n", payload);
            break;

        default:
            break;
    }
}

void setup() {
    Serial.begin(115200);

    // Enable AP+STA mode
    WiFi.mode(WIFI_AP_STA);

    setupSoftAP();

    b_wifiConnected = connectToWiFi();
    if (b_wifiConnected) {
        setupWebSocket();
    }

    setupAsyncWebServer();
}

void loop() {
    // WiFi reconnection handling
    if (WiFi.status() != WL_CONNECTED) {
        if (b_wifiConnected) {
            Serial.println("Disconnected from WiFi, attempting to reconnect...");
            b_wifiConnected = false;
            ms_wifiReconnectDelay.start(10000);
        } else if (ms_wifiReconnectDelay.justFinished()) {
            b_wifiConnected = connectToWiFi();
            if (b_wifiConnected) {
                setupWebSocket();
            }
        }
    } else if (!b_wifiConnected) {
        Serial.println("Reconnected to WiFi.");
        b_wifiConnected = true;
    }

    // WebSocket reconnection handling
    if (b_wifiConnected && !b_webSocketConnected) {
        if (ms_webSocketReconnectDelay.justFinished()) {
            Serial.println("Attempting WebSocket reconnection...");
            setupWebSocket();
        }
    }

    // WebSocket client loop
    if (b_wifiConnected && b_webSocketConnected) {
        webSocket.loop();
    }

    delay(10);  // Small delay to avoid high CPU usage
}
