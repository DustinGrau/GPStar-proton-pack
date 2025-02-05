#include <Arduino.h>

// Define SPI pins
#define PIN_MOSI 23  
#define PIN_MISO 19  
#define PIN_SCLK 18  
#define PIN_CS   5   

// Enum for SPI modes
enum SPIMode {
    MODE0, // CPOL = 0, CPHA = 0 (Clock idle LOW, data captured on rising edge, output on falling edge)
    MODE1, // CPOL = 0, CPHA = 1 (Clock idle LOW, data captured on falling edge, output on rising edge)
    MODE2, // CPOL = 1, CPHA = 0 (Clock idle HIGH, data captured on falling edge, output on rising edge)
    MODE3  // CPOL = 1, CPHA = 1 (Clock idle HIGH, data captured on rising edge, output on falling edge)
};

SPIMode currentMode = MODE0; // Default: SPI Mode 0

// Enum for bit order
enum BitOrder {
    MSB_FIRST,
    LSB_FIRST
};

BitOrder currentBitOrder = MSB_FIRST; // Default: MSB-first

// Circular buffer settings
const int BUFFER_SIZE = 128;
volatile uint16_t circularBufferMOSI[BUFFER_SIZE];
volatile uint16_t circularBufferMISO[BUFFER_SIZE];
volatile bool csActive = false;
volatile int bufferHead = 0;
volatile int bufferTail = 0;

#define TRANSACTION_END_MARKER 0xFFF

enum OUT_Format { OUT_HEX, OUT_DECIMAL, OUT_ASCII, OUT_BINARY };

// Process and send SPI data with format options
void processSPIData(OUT_Format format = OUT_HEX) {
    if (bufferHead == bufferTail) return; // No data
    
    String message = "";
    while (bufferHead != bufferTail) {
        uint16_t mosiData = circularBufferMOSI[bufferTail];
        uint16_t misoData = circularBufferMISO[bufferTail];
        bufferTail = (bufferTail + 1) % BUFFER_SIZE;
        
        if (mosiData == TRANSACTION_END_MARKER && misoData == TRANSACTION_END_MARKER) {
            Serial.println(); // Transaction boundary
            continue;
        }
        
        switch (format) {
            case OUT_HEX:
                message += "0x" + String(mosiData, HEX) + " --> <-- 0x" + String(misoData, HEX) + "\n";
                break;
            case OUT_DECIMAL:
                message += String(mosiData) + " --> <-- " + String(misoData) + "\n";
                break;
            case OUT_ASCII:
                message += "'" + String((char)mosiData) + "' --> <-- '" + String((char)misoData) + "'\n";
                break;
            case OUT_BINARY:
                message += "0b" + String(mosiData, BIN) + " --> <-- 0b" + String(misoData, BIN) + "\n";
                break;
        }
    }
    Serial.print(message);
}

// Interrupt on SPI clock signal
void IRAM_ATTR onClockEdge() {
    bool clockRising = digitalRead(PIN_SCLK);
    bool captureData = false;
    
    switch (currentMode) {
        case MODE0: captureData = clockRising; break;
        case MODE1: captureData = !clockRising; break;
        case MODE2: captureData = !clockRising; break;
        case MODE3: captureData = clockRising; break;
    }
    
    static uint8_t byteBufferMOSI = 0;
    static uint8_t byteBufferMISO = 0;
    static uint8_t bitCount = 0;
    
    if (captureData) {
        if (currentBitOrder == MSB_FIRST) {
            byteBufferMOSI <<= 1;
            byteBufferMISO <<= 1;
            if (digitalRead(PIN_MOSI)) byteBufferMOSI |= 1;
            if (digitalRead(PIN_MISO)) byteBufferMISO |= 1;
        } else {
            byteBufferMOSI >>= 1;
            byteBufferMISO >>= 1;
            if (digitalRead(PIN_MOSI)) byteBufferMOSI |= 0x80;
            if (digitalRead(PIN_MISO)) byteBufferMISO |= 0x80;
        }
    }
    
    bitCount++;
    if (bitCount == 8) {
        bitCount = 0;
        int nextHead = (bufferHead + 1) % BUFFER_SIZE;
        if (nextHead != bufferTail) { // Ensure space in buffer
            circularBufferMOSI[bufferHead] = byteBufferMOSI;
            circularBufferMISO[bufferHead] = byteBufferMISO;
            bufferHead = nextHead;
        }
    }
}

// Interrupt on CS pin to detect transaction start and end
void IRAM_ATTR onCSEdge() {
    csActive = !digitalRead(PIN_CS);
    if (!csActive) {
        int nextHead = (bufferHead + 1) % BUFFER_SIZE;
        if (nextHead != bufferTail) {
            circularBufferMOSI[bufferHead] = TRANSACTION_END_MARKER;
            circularBufferMISO[bufferHead] = TRANSACTION_END_MARKER;
            bufferHead = nextHead;
        }
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(PIN_MOSI, INPUT);
    pinMode(PIN_MISO, INPUT);
    pinMode(PIN_SCLK, INPUT);
    pinMode(PIN_CS, INPUT);
    
    attachInterrupt(digitalPinToInterrupt(PIN_SCLK), onClockEdge, CHANGE);
    attachInterrupt(digitalPinToInterrupt(PIN_CS), onCSEdge, CHANGE);
    
    Serial.println("SPI Monitor Initialized.");
}

void loop() {    
    processSPIData(OUT_HEX);
} 
