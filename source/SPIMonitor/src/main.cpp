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

// SPI monitoring variables
volatile uint8_t byteBufferMOSI = 0;
volatile uint8_t byteBufferMISO = 0;
volatile uint8_t bitCount = 0;
volatile bool newByteAvailable = false;
volatile bool csActive = false;
volatile bool transactionComplete = false;

// Buffer for transaction data
const int MAX_TRANSACTION_SIZE = 64;
volatile uint8_t transactionMOSI[MAX_TRANSACTION_SIZE];
volatile uint8_t transactionMISO[MAX_TRANSACTION_SIZE];
volatile uint8_t transactionLength = 0;

enum OUT_Format { OUT_HEX, OUT_DECIMAL, OUT_ASCII, OUT_BINARY };

// Process and send SPI data with format options
void processSPIData(OUT_Format format = OUT_HEX) {
    if (transactionLength == 0) {
        Serial.println(); // Print newline if no data
        return;
    }
    
    String message = "";
    for (int i = 0; i < transactionLength; i++) {
        if (transactionMOSI[i] == 0 && transactionMISO[i] == 0) {
          Serial.println(); // Print newline if no data
        }
        else {
          switch (format) {
              case OUT_HEX:
                  message += "0x" + String(transactionMOSI[i], HEX) + " --> <-- 0x" + String(transactionMISO[i], HEX) + "\n";
                  break;
              case OUT_DECIMAL:
                  message += String(transactionMOSI[i]) + " --> <-- " + String(transactionMISO[i]) + "\n";
                  break;
              case OUT_ASCII:
                  message += "'" + String((char)transactionMOSI[i]) + "' --> <-- '" + String((char)transactionMISO[i]) + "'\n";
                  break;
              case OUT_BINARY:
                  message += "0b" + String(transactionMOSI[i], BIN) + " --> <-- 0b" + String(transactionMISO[i], BIN) + "\n";
                  break;
          }
        }
    }
    Serial.print(message);
}

// Interrupt on SPI clock signal
void IRAM_ATTR onClockEdge() {
    bool clockRising = digitalRead(PIN_SCLK); // True if clock transitions from LOW to HIGH

    // Determine data capture based on SPI mode
    bool captureData = false;
    switch (currentMode) {
        case MODE0: captureData = clockRising; break;  // Capture on rising edge
        case MODE1: captureData = !clockRising; break; // Capture on falling edge
        case MODE2: captureData = !clockRising; break; // Capture on falling edge
        case MODE3: captureData = clockRising; break;  // Capture on rising edge
    }

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
        newByteAvailable = true;
        if (transactionLength < MAX_TRANSACTION_SIZE) {
            transactionMOSI[transactionLength] = byteBufferMOSI;
            transactionMISO[transactionLength] = byteBufferMISO;
            transactionLength++;
        }
        byteBufferMOSI = 0;
        byteBufferMISO = 0;
    }
}

// Interrupt on CS pin to detect transaction start and end
void IRAM_ATTR onCSEdge() {
    csActive = !digitalRead(PIN_CS);
    if (!csActive) {
        transactionComplete = true;
    } else {
        transactionLength = 0; // Reset buffer on new transaction
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
    if (transactionComplete) {
        processSPIData(OUT_HEX);
        transactionComplete = false;
    }
}
