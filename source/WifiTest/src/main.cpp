// Required for PlatformIO
#include <Arduino.h>

#include <esp_system.h>
#include <nvs_flash.h>
#include <Preferences.h>
#include <millisDelay.h>
#include <WiFi.h>
#include <WiFiAP.h>
#include <ESPmDNS.h>
#include <AsyncJson.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>
#include <WebSocketsClient.h>

// WiFi settings
const char* ssid = "BenchRig";
const char* password = "12345678";
const char* softAP_ssid = "WifiTest";
const char* softAP_password = "12345678";

// WebSocket server settings
const char* websocket_host = "192.168.1.2";
const uint16_t websocket_port = 80;
const char* websocket_uri = "/ws";

// WiFi and WebSocket client objects
WiFiClient wifiClient;
WebSocketsClient webSocket;

// Variables to track WiFi and WebSocket connection state
bool wifiConnected = false;
bool webSocketConnected = false;

// Delay object for non-blocking WiFi reconnection
millisDelay wifiReconnectDelay;

// Function to connect to external WiFi network, returns true on success
bool connectToWiFi() {
    Serial.printf("Connecting to %s...\n", ssid);
    WiFi.begin(ssid, password);

    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
        // Use a non-blocking approach here as well
        if (millis() - startAttemptTime >= 500) {
            Serial.print(".");
            startAttemptTime = millis(); // reset counter for next dot display
        }
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected to external WiFi.");
        return true;
    } else {
        Serial.println("\nFailed to connect to WiFi. Retrying...");
        return false;
    }
}

// WebSocket event handler
void webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
        case WStype_DISCONNECTED:
            Serial.println("WebSocket disconnected. Attempting to reconnect...");
            webSocketConnected = false;
            webSocket.begin(websocket_host, websocket_port, websocket_uri);
            break;

        case WStype_CONNECTED:
            Serial.printf("WebSocket connected to %s:%d%s\n", websocket_host, websocket_port, websocket_uri);
            webSocketConnected = true;
            webSocket.sendTXT("Hello, server!"); // Example message to send on connection
            break;

        case WStype_TEXT:
            Serial.printf("Received message: %s\n", payload);
            break;

        default:
            break;
    }
}

// Function to setup WebSocket connection
void setupWebSocket() {
    Serial.println("Initializing WebSocket connection...");
    webSocket.begin(websocket_host, websocket_port, websocket_uri);
    webSocket.onEvent(webSocketEvent);
    webSocketConnected = true;
}

void setup() {
    Serial.begin(115200);

    // Start SoftAP network
    Serial.println("Starting SoftAP network...");
    WiFi.softAP(softAP_ssid, softAP_password);
    Serial.println("SoftAP started.");

    // Attempt to connect to external WiFi
    wifiConnected = connectToWiFi();

    // Initialize WebSocket only if connected to WiFi
    if (wifiConnected) {
        setupWebSocket();
    }
}

void loop() {
    // WiFi reconnection handling
    if (WiFi.status() != WL_CONNECTED) {
        if (wifiConnected) {
            Serial.println("Disconnected from external WiFi, reconnecting...");
            wifiConnected = false;
            wifiReconnectDelay.start(10000); // Start 10-second delay before reconnection
        } else if (wifiReconnectDelay.justFinished()) {
            wifiConnected = connectToWiFi();
            if (wifiConnected && !webSocketConnected) {
                setupWebSocket(); // Reinitialize WebSocket when WiFi reconnects
            }
        }
    } else if (!wifiConnected) {
        Serial.println("Reconnected to external WiFi.");
        wifiConnected = true;
    }

    // WebSocket client loop if connected to WiFi
    if (wifiConnected) {
        webSocket.loop();
    }

    delay(10); // Small delay to avoid high CPU usage
}
