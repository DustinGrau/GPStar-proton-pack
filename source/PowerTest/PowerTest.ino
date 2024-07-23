// Set to 1 to enable built-in debug messages
#define DEBUG 1

// Debug macros
#if DEBUG == 1
#define debug(x) Serial.print(x)
#define debugln(x) Serial.println(x)
#else
#define debug(x)
#define debugln(x)
#endif

// Turn on debugging for the library
#define INA219_DEBUG

#include <millisDelay.h>
#include <Wire.h>
#include <INA219.h> // https://github.com/flav1972/ArduinoINA219

#define SHUNT_R     0.015 // Shunt resistor in ohms
#define SHUNT_MAX_V 0.075 // Max based on 75mV for 50A current 
#define BUS_MAX_V   16.0  // Plenty for 5V nominal voltage
#define MAX_CURRENT 3.2   // Stated maximum is 3.2A

INA219 monitor; // Power monitor object on i2c bus using the INA219 chip.
boolean b_power_meter = false; // Whether a power meter device exists on i2c bus.
const uint8_t i_power_reading_delay = 50; // How often to read the power levels (ms).
const uint16_t i_power_display_delay = 1000; // How often to display the power levels.
millisDelay ms_power_reading; // Timer for reading latest values from power meter.
millisDelay ms_power_display; // Timer for generating output from power readings.

// Power Values
int16_t i_ShuntVoltageRaw = 0;
int16_t i_BusVoltageRaw = 0;
float f_ShuntVoltage = 0; // mV
float f_ShuntCurrent = 0; // A
float f_BusVoltage = 0; // V
float f_BattVoltage = 0; // V
float f_BusPower = 0; // mW
float f_AmpHours = 0; // Ah
unsigned long i_power_last_read = 0; // Used to calculate Ah est.
unsigned long i_power_read_tick; // Current read time - last read

void setup(){
  Serial.begin(9600);

  uint8_t i_monitor_status = monitor.begin();
  debugln("");
  debug(F("Power Meter Result: "));
  debugln(i_monitor_status);
  if (i_monitor_status > 0){
    // If returning a non-zero value, device could not be reset.
    debugln(F("Unable to find power monitoring device."));
  }
  else {
    powerConfig();
    i_power_last_read = millis(); // Used with 
    ms_power_reading.start(i_power_reading_delay);
    ms_power_display.start(i_power_display_delay);
  }
}

void loop(){
  if(b_power_meter){
    if(ms_power_reading.justFinished()){
      powerReading();
      ms_power_reading.start(i_power_reading_delay);
    }

    if(ms_power_display.justFinished()){
      powerDisplay();
      ms_power_display.start(i_power_display_delay);
    }
  }
}

void powerConfig(){
  debugln(F("Configure Power Meter"));

  b_power_meter = true;

  // Set a custom configuration, default values are RANGE_32V, GAIN_8_320MV, ADC_12BIT, ADC_12BIT, CONT_SH_BUS
  monitor.configure(INA219::RANGE_16V, INA219::GAIN_2_80MV, INA219::ADC_64SAMP, INA219::ADC_64SAMP, INA219::CONT_SH_BUS);
  
  // Calibrate with our chosen values
  monitor.calibrate(SHUNT_R, SHUNT_MAX_V, BUS_MAX_V, MAX_CURRENT);
}

void powerReading(){
  if (!b_power_meter) { return; }

  //debugln(F("Reading Power Meter"));

  unsigned long i_new_time;

  // Reads the latest values from the monitor.  
  i_ShuntVoltageRaw = monitor.shuntVoltageRaw();
  i_BusVoltageRaw = monitor.busVoltageRaw();
  f_ShuntVoltage = monitor.shuntVoltage() * 1000;
  f_ShuntCurrent = monitor.shuntCurrent();
  f_BusVoltage = monitor.busVoltage();
  f_BattVoltage = f_BusVoltage + (f_ShuntVoltage / 1000);
  f_BusPower = monitor.busPower() * 1000;

  // Use time and values to calculate Ah estimate.
  i_new_time = millis();
  i_power_read_tick = i_new_time - i_power_last_read;
  f_AmpHours += (f_ShuntCurrent * i_power_read_tick) / 3600000.0;
  i_power_last_read = i_new_time;

  // Prepare for next read -- this is security just in case the ina219 is reset by transient current
  monitor.recalibrate();
  monitor.reconfig();
}

void powerDisplay(){
  if (!b_power_meter) { return; }

  debugln(F("Display Power Values"));

  // Displays the latest gathered values.
  Serial.println("******************");
  
  Serial.print("Raw Shunt Voltage: ");
  Serial.println(i_ShuntVoltageRaw);
  
  Serial.print("Raw Bus Voltage:   ");
  Serial.println(i_BusVoltageRaw);
  
  Serial.println("--");
  
  Serial.print("Shunt Voltage: ");
  Serial.print(f_ShuntVoltage, 4);
  Serial.println(" mV");
  
  Serial.print("Shunt Current: ");
  Serial.print(f_ShuntCurrent, 4);
  Serial.println(" A");
  
  Serial.print("Bus Voltage:   ");
  Serial.print(f_BusVoltage, 4);
  Serial.println(" V");

  Serial.print("Batt Voltage:  ");
  Serial.print(f_BattVoltage, 4);
  Serial.println(" V");

  Serial.print("Bus Power:     ");
  Serial.print(f_BusPower, 4);
  Serial.println(" mW");
  
  Serial.print("Amp Hours:     ");
  Serial.print(f_AmpHours, 4);
  Serial.println(" Ah");

  Serial.println(" ");
  Serial.println(" ");
}