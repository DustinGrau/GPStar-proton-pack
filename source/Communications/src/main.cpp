#include <Arduino.h>

// Define SPI pins
#define PIN_MOSI 23  
#define PIN_MISO 19  
#define PIN_SCLK 18  
#define PIN_CS   5   

// Variables for SPI monitoring
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

void setup() {
    Serial.begin(115200);
    pinMode(PIN_MOSI, INPUT);
    pinMode(PIN_MISO, INPUT);
    pinMode(PIN_SCLK, INPUT);
    pinMode(PIN_CS, INPUT);

    attachInterrupt(digitalPinToInterrupt(PIN_SCLK), onClockEdge, RISING);
}

void loop() {
    if (newByteAvailable) {
        bool csState = digitalRead(PIN_CS);  
        char mosiChar = (byteBufferMOSI >= 32 && byteBufferMOSI <= 126) ? (char)byteBufferMOSI : '?';
        char misoChar = (byteBufferMISO >= 32 && byteBufferMISO <= 126) ? (char)byteBufferMISO : '?';

        // Show CS state when LOW (active communication)
        if (!csState) Serial.print("[CS LOW] ");

        // Display Master to Slave communication
        Serial.print("MOSI -> 0x");
        Serial.print(byteBufferMOSI, HEX);
        Serial.print(" ('");
        Serial.print(mosiChar);
        Serial.print("') ");

        // Display Slave to Master response (if any)
        if (byteBufferMISO != 0) {
            Serial.print("| MISO <- 0x");
            Serial.print(byteBufferMISO, HEX);
            Serial.print(" ('");
            Serial.print(misoChar);
            Serial.print("')");
        }

        Serial.println();
        newByteAvailable = false;
    }
}
