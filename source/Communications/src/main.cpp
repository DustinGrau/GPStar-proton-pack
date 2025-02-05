#include <Arduino.h>
#include <SPI.h>

// Define SPI pins
#define PIN_MOSI 23  
#define PIN_MISO 19  
#define PIN_SCLK 18  
#define PIN_CS   5   

// SPI settings
#define SPI_CLOCK 20000000 // 20 MHz
#define SPI_MODE  SPI_MODE0

bool dataSent = false;

void setup() {
    Serial.begin(115200);
    SPI.begin(PIN_SCLK, PIN_MISO, PIN_MOSI, PIN_CS);
    pinMode(PIN_CS, OUTPUT);
    digitalWrite(PIN_CS, HIGH); // Ensure CS is high initially
    Serial.println("SPI Master Initialized");
}

void sendSPICommand(uint8_t command) {
    digitalWrite(PIN_CS, LOW); // Select the slave
    SPI.beginTransaction(SPISettings(SPI_CLOCK, MSBFIRST, SPI_MODE));
    
    SPI.transfer(command);
    
    SPI.endTransaction();
    digitalWrite(PIN_CS, HIGH); // Deselect the slave
    Serial.print("Sent SPI Command: 0x");
    Serial.println(command, HEX);
}

void loop() {
    // Do nothing after sending the command sequence once
    if (!dataSent) {
        sendSPICommand(0x1);
        sendSPICommand(0x0);
        sendSPICommand(0x0);
        dataSent = true;
    }
}
