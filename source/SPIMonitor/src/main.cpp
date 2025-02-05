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

SPIMode currentMode = MODE0; // Default SPI mode

// Enum for bit order
enum BitOrder {
    MSB_FIRST,
    LSB_FIRST
};

BitOrder currentBitOrder = MSB_FIRST; // Default to MSB-first

// SPI monitoring variables
volatile uint8_t byteBufferMOSI = 0;
volatile uint8_t byteBufferMISO = 0;
volatile uint8_t bitCount = 0;
volatile bool newByteAvailable = false;
volatile bool csActive = false;
volatile bool transactionComplete = false;

// Transaction buffer
String transactionBuffer = "";

// Timing variables for clock frequency estimation
volatile unsigned long lastClockTime = 0;
volatile unsigned long clockPeriodSum = 0;
volatile uint8_t clockEdgeCount = 0;
float estimatedClockFreq = 0.0;

// Interrupt on SPI clock signal
void IRAM_ATTR onClockEdge() {
    unsigned long currentTime = micros();
    bool clockRising = digitalRead(PIN_SCLK); // True if clock transitions from LOW to HIGH

    // Estimate clock frequency
    if (lastClockTime > 0) {
        clockPeriodSum += (currentTime - lastClockTime);
        clockEdgeCount++;
    }
    lastClockTime = currentTime;

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
    }
}

// Interrupt on CS pin to detect transaction start and end
void IRAM_ATTR onCSEdge() {
    csActive = !digitalRead(PIN_CS);
    if (!csActive) {
        transactionComplete = true;
        if (clockEdgeCount > 0) {
            estimatedClockFreq = 1e6 / (clockPeriodSum / (float)clockEdgeCount);
            clockPeriodSum = 0;
            clockEdgeCount = 0;
        }
    }
}

// Process and send SPI data with format options
enum OUT_Format { OUT_HEX, OUT_DECIMAL, OUT_ASCII, OUT_BINARY };
void processSPIData(OUT_Format format = OUT_HEX) {
    if (newByteAvailable) {
        String message = "";
        switch (format) {
            case OUT_HEX:
                message = "0x" + String(byteBufferMOSI, HEX) + " --> <-- 0x" + String(byteBufferMISO, HEX);
                break;
            case OUT_DECIMAL:
                message = String(byteBufferMOSI) + " --> <-- " + String(byteBufferMISO);
                break;
            case OUT_ASCII:
                message = "'" + String((char)byteBufferMOSI) + "' --> <-- '" + String((char)byteBufferMISO) + "'";
                break;
            case OUT_BINARY:
                message = "0b" + String(byteBufferMOSI, BIN) + " --> <-- 0b" + String(byteBufferMISO, BIN);
                break;
        }
        transactionBuffer += message + "\n";
        newByteAvailable = false;
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
    
    if (transactionComplete) {
        if (transactionBuffer.length() > 0) {
            Serial.println(transactionBuffer);
            Serial.print("Estimated Clock Frequency: ");
            Serial.print(estimatedClockFreq);
            Serial.println(" Hz");
            transactionBuffer = "";
        }
        transactionComplete = false;
    }
}
