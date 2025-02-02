#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>

// WiFi credentials
const char* ssid = "Jurai";
const char* password = "8978795077";

// Define SPI pins
#define PIN_MOSI 23  
#define PIN_MISO 19  
#define PIN_SCLK 18  
#define PIN_CS   5   

// Web server and WebSockets
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// SPI monitoring variables
volatile uint8_t byteBufferMOSI = 0;
volatile uint8_t byteBufferMISO = 0;
volatile uint8_t bitCount = 0;
volatile bool newByteAvailable = false;

// Interrupt on SPI clock signal
void IRAM_ATTR onClockEdge() {
    byteBufferMOSI <<= 1;
    byteBufferMISO <<= 1;

    if (digitalRead(PIN_MOSI)) byteBufferMOSI |= 1;
    if (digitalRead(PIN_MISO)) byteBufferMISO |= 1;
    
    bitCount++;  
    if (bitCount == 8) {
        bitCount = 0;
        newByteAvailable = true;
    }
}

// Process and send SPI data
void processSPIData() {
    if (newByteAvailable) {
        bool csActive = !digitalRead(PIN_CS);
        String message = "";

        if (csActive) {
            message += "[TRANSACTION START]\n";
        }

        char mosiChar = (byteBufferMOSI >= 32 && byteBufferMOSI <= 126) ? (char)byteBufferMOSI : '?';
        char misoChar = (byteBufferMISO >= 32 && byteBufferMISO <= 126) ? (char)byteBufferMISO : '?';

        message += "MOSI -> 0x" + String(byteBufferMOSI, HEX) + " ('" + String(mosiChar) + "') ";
        if (byteBufferMISO != 0) {
            message += "| MISO <- 0x" + String(byteBufferMISO, HEX) + " ('" + String(misoChar) + "')";
        }

        ws.textAll(message);
        //Serial.println(message);
        newByteAvailable = false;
    }
}

void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.println("[WebSocket] Client Connected");
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(PIN_MOSI, INPUT);
    pinMode(PIN_MISO, INPUT);
    pinMode(PIN_SCLK, INPUT);
    pinMode(PIN_CS, INPUT);

    attachInterrupt(digitalPinToInterrupt(PIN_SCLK), onClockEdge, RISING);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected");
    Serial.print("ESP32 IP Address: ");
    Serial.println(WiFi.localIP());

    if (MDNS.begin("esp32-spi")) {
        Serial.println("mDNS responder started: http://esp32-spi.local");
    }

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/html", "<html><body><h2>SPI Monitor</h2><pre id='log'></pre><script>let ws=new WebSocket('ws://esp32-spi.local/ws');ws.onmessage=e=>document.getElementById('log').innerHTML+=e.data+'\\n';</script></body></html>");
    });

    ws.onEvent(onWebSocketEvent);
    server.addHandler(&ws);
    server.begin();
}

void loop() {
    processSPIData();
}
