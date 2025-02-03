#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>

// WiFi credentials
// const char* ssid = "SSID";
// const char* password = "PASSWORD";

// Define SPI pins
#define PIN_MOSI 23  
#define PIN_MISO 19  
#define PIN_SCLK 18  
#define PIN_CS   5   

// Web server and WebSockets
// AsyncWebServer server(80);
// AsyncWebSocket ws("/ws");

// SPI monitoring variables
volatile uint8_t bitCount = 0;
volatile uint8_t byteBufferMOSI = 0;
volatile uint8_t byteBufferMISO = 0;
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
    bool csActive = !digitalRead(PIN_CS); // Active = CS is LOW (Master is actively communicating to Slave)
    String message = "";

    if (csActive) {
      message += "[TRANSACTION START]\n";
    }

    char mosiChar = (byteBufferMOSI >= 32 && byteBufferMOSI <= 126) ? (char)byteBufferMOSI : '?';
    char misoChar = (byteBufferMISO >= 32 && byteBufferMISO <= 126) ? (char)byteBufferMISO : '?';

    message += "M.Send: 0x" + String(byteBufferMOSI, HEX);
    // message += " (" + String(byteBufferMOSI, DEC) + ")";
    // message += " [" + String(byteBufferMOSI, BIN) + "]";
    if (byteBufferMISO != 0) {
      message += " | S.Resp: 0x" + String(byteBufferMISO, HEX);
      // message += " (" + String(byteBufferMISO, DEC) + ")";
      // message += " [" + String(byteBufferMOSI, BIN) + "]";
    }

    // ws.textAll(message);
    Serial.println(message);
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

  // Set pins as input so we can snoop on the bus data.
  pinMode(PIN_MOSI, INPUT);
  pinMode(PIN_MISO, INPUT);
  pinMode(PIN_SCLK, INPUT);
  pinMode(PIN_CS, INPUT);

  // SPI Mode 0 (CPOL=0, CPHA=0)
  attachInterrupt(digitalPinToInterrupt(PIN_SCLK), onClockEdge, RISING);

  // SPI Mode 1 (CPOL=0, CPHA=1)
  // attachInterrupt(digitalPinToInterrupt(PIN_SCLK), onClockEdge, FALLING);

  // Initialize connection to local WiFi network.
  // WiFi.begin(ssid, password);
  // while (WiFi.status() != WL_CONNECTED) {
  //   delay(500);
  //   Serial.print(".");
  // }
  // Serial.println("\nWiFi connected");
  // Serial.print("ESP32 IP Address: ");
  // Serial.println(WiFi.localIP());

  // if (MDNS.begin("esp32-spi")) {
  //   Serial.println("mDNS responder started: http://esp32-spi.local");
  // }

  // server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
  //   request->send(200, "text/html", "<html><body><h2>SPI Monitor</h2><pre id='log'></pre><script>let ws=new WebSocket('ws://esp32-spi.local/ws');ws.onmessage=e=>document.getElementById('log').innerHTML+=e.data+'\\n';</script></body></html>");
  // });

  // ws.onEvent(onWebSocketEvent);
  // server.addHandler(&ws);
  // server.begin();
}

void loop() {
  processSPIData();
}
