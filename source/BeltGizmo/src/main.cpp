/**
 *   GPStar BeltGizmo - Ghostbusters Props, Mods, and Kits.
 *   Copyright (C) 2024 Dustin Grau <dustin.grau@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, see <https://www.gnu.org/licenses/>.
 *
 */

// Required for PlatformIO
#include <Arduino.h>

// Suppress warning about SPI hardware pins
// Define this before including <FastLED.h>
#define FASTLED_INTERNAL

// PROGMEM macro
#define PROGMEM_READU32(x) pgm_read_dword_near(&(x))
#define PROGMEM_READU16(x) pgm_read_word_near(&(x))
#define PROGMEM_READU8(x) pgm_read_byte_near(&(x))

// 3rd-Party Libraries
#include <millisDelay.h>
#include <FastLED.h>
#include <esp_system.h>
#include <nvs_flash.h>

// Local Files
#include "Configuration.h"
#include "Header.h"
#include "Colours.h"
#include "Wireless.h"
#include "System.h"

// Task Handles
TaskHandle_t AnimationTaskHandle = NULL;
TaskHandle_t PreferencesTaskHandle = NULL;
TaskHandle_t UserInputTaskHandle = NULL;
TaskHandle_t WiFiManagementTaskHandle = NULL;
TaskHandle_t WiFiSetupTaskHandle = NULL;

// Variables for approximating CPU load
// https://www.arduino.cc/reference/en/language/variables/variable-scope-qualifiers/volatile/
volatile uint32_t idleTimeCore0 = 0;
volatile uint32_t idleTimeCore1 = 0;

// Idle task for Core 0
#if defined(DEBUG_PERFORMANCE)
void idleTaskCore0(void * parameter) {
  while(true) {
    idleTimeCore0 = idleTimeCore0 + 1;
    vTaskDelay(1);
  }
}
#endif

// Idle task for Core 1
#if defined(DEBUG_PERFORMANCE)
void idleTaskCore1(void * parameter) {
  while(true) {
    idleTimeCore1 = idleTimeCore1 + 1;
    vTaskDelay(1);
  }
}
#endif

// Animation Task (Loop)
void AnimationTask(void *parameter) {
  while(true) {
    #if defined(DEBUG_TASK_TO_CONSOLE)
      // Confirm the core in use for this task, and when it runs.
      Serial.print(F("Executing AnimationTask in core"));
      Serial.print(xPortGetCoreID());
      // Get the stack high water mark for optimizing bytes allocated.
      Serial.print(F(" | Stack HWM: "));
      Serial.println(uxTaskGetStackHighWaterMark(NULL));
    #endif

    // Update blinking lights based on websocket data.
    //blinkLights();
    animateLights();

    // Update the device LEDs and restart the timer.
    FastLED.show();

    vTaskDelay(8 / portTICK_PERIOD_MS); // 8ms delay
  }
}

// Preferences Task (Single-Run)
void PreferencesTask(void *parameter) {
  #if defined(DEBUG_TASK_TO_CONSOLE)
    // Confirm the core in use for this task, and when it runs.
    Serial.print(F("Executing PreferencesTask in core"));
    Serial.println(xPortGetCoreID());
  #endif

  // Print partition information to verify NVS availability
  #if defined(DEBUG_SEND_TO_CONSOLE)
  printPartitions();
  #endif

  // Initialize the NVS flash partition and throw any errors as necessary.
  esp_err_t err = nvs_flash_init();
  if(err != ESP_OK) {
    #if defined(DEBUG_SEND_TO_CONSOLE)
    Serial.printf("NVS initialization failed with error: %s\n", esp_err_to_name(err));
    #endif

    // If initialization fails, erase and reinitialize NVS.
    debug(F("Erasing and reinitializing NVS..."));
    nvs_flash_erase();

    err = nvs_flash_init();
    if(err != ESP_OK) {
      #if defined(DEBUG_SEND_TO_CONSOLE)
      Serial.printf("Failed to reinitialize NVS: %s\n", esp_err_to_name(err));
      #endif
    }
    else {
      debug(F("NVS reinitialized successfully"));
    }
  }
  else {
    debug(F("NVS initialized successfully"));
  }

  #if defined(DEBUG_TASK_TO_CONSOLE)
    // Get the stack high water mark for optimizing bytes allocated.
    Serial.print(F("PreferencesTask Stack HWM: "));
    Serial.println(uxTaskGetStackHighWaterMark(NULL));
  #endif

  // Task ends after setup is complete and MUST be removed from scheduling.
  // Failure to do this can cause an error within the watchdog timer!
  vTaskDelete(NULL);
}

// User Input Task (Loop)
void UserInputTask(void *parameter) {
  while(true) {
    #if defined(DEBUG_TASK_TO_CONSOLE)
      // Confirm the core in use for this task, and when it runs.
      Serial.print(F("Executing UserInputTask in core"));
      Serial.print(xPortGetCoreID());
      // Get the stack high water mark for optimizing bytes allocated.
      Serial.print(F(" | Stack HWM: "));
      Serial.println(uxTaskGetStackHighWaterMark(NULL));
    #endif

    vTaskDelay(14 / portTICK_PERIOD_MS); // 14ms delay
  }
}

// WiFi Management Task (Loop)
void WiFiManagementTask(void *parameter) {
  while(true) {
    #if defined(DEBUG_TASK_TO_CONSOLE)
      // Confirm the core in use for this task, and when it runs.
      Serial.print(F("Executing WiFiManagementTask in core"));
      Serial.print(xPortGetCoreID());
      // Get the stack high water mark for optimizing bytes allocated.
      Serial.print(F(" | Stack HWM: "));
      Serial.println(uxTaskGetStackHighWaterMark(NULL));
    #endif

    // Handle reconnection to external WiFi when necessary.
    if (b_ap_started) {
      /*
      if (WiFi.status() != WL_CONNECTED) {
        Serial.println(F("WiFi Connection Lost"));

        // If wifi has dropped, clear some flags.
        b_ext_wifi_started = false;
        b_socket_ready = false;

        // Try to reconnect then check status.
        Serial.println(F("WiFi Restart"));
        //WiFi.disconnect(); // Explicitly issue a disconnect.
        delay(100); // Delay needed before WiFi restart.
        b_ext_wifi_started = startExternalWifi();
        if (b_ext_wifi_started) {
          setupWebSocket(); // Restore the WebSocket connection.
        }
      }
      */
      // WL_IDLE_STATUS      = 0,
      // WL_NO_SSID_AVAIL    = 1,
      // WL_SCAN_COMPLETED   = 2,
      // WL_CONNECTED        = 3,
      // WL_CONNECT_FAILED   = 4,
      // WL_CONNECTION_LOST  = 5,
      // WL_DISCONNECTED     = 6
      if (WiFi.status() == WL_CONNECTED && b_ext_wifi_started && !b_socket_ready) {
        Serial.println(F("WiFi Connected, Socket Not Configured"));
        setupWebSocket(); // Restore the WebSocket connection.
      }
    }

    // Proceed with management if the AP and web server are started.
    if(b_ap_started && b_ws_started) {
      if(ms_otacheck.remaining() < 1) {
        // Handles device reboot after an OTA update.
        ElegantOTA.loop();

        // Restart timer for next check.
        ms_otacheck.start(i_otaCheck);
      }

      // Try to start the external WiFi.
      if(!b_ext_wifi_started) {
        b_ext_wifi_started = startExternalWifi();
      }
    }

    vTaskDelay(2000 / portTICK_PERIOD_MS); // 2000ms delay
  }
}

// WiFi Setup Task (Single-Run)
void WiFiSetupTask(void *parameter) {
  #if defined(DEBUG_TASK_TO_CONSOLE)
    // Confirm the core in use for this task, and when it runs.
    Serial.print(F("Executing WiFiSetupTask in core"));
    Serial.println(xPortGetCoreID());
  #endif

  // Begin by setting up WiFi as a prerequisite to all else.
  if(startWiFi()) {
    // Start the local web server.
    startWebServer();

    // Begin timer for remote update events.
    ms_otacheck.start(i_otaCheck);
  }

  #if defined(DEBUG_TASK_TO_CONSOLE)
    // Get the stack high water mark for optimizing bytes allocated.
    Serial.print(F("WiFiSetupTask Stack HWM: "));
    Serial.println(uxTaskGetStackHighWaterMark(NULL));
  #endif

  // Task ends after setup is complete and MUST be removed from scheduling.
  // Failure to do this can cause an error within the watchdog timer!
  vTaskDelete(NULL);
}

void setup() {
  //Serial.begin(115200); // Serial monitor via USB connection.
  Serial.begin(115200); // Serial monitor via USB connection.
  delay(1000); // Provide a delay to allow serial output.

  // Provide an opportunity to set the CPU Frequency MHz: 80, 160, 240 [Default = 240]
  // Lower frequency means less power consumption, but slower performance (obviously).
  setCpuFrequencyMhz(160);
  #if defined(DEBUG_SEND_TO_CONSOLE)
    Serial.print(F("CPU Freq (MHz): "));
    Serial.println(getCpuFrequencyMhz());
  #endif

  // Boot into proton mode at level 1 by default.
  STREAM_MODE = PROTON;
  POWER_LEVEL = LEVEL_1;

  // Device RGB LEDs for use when needed.
  FastLED.addLeds<NEOPIXEL, DEVICE_LED_PIN>(device_leds, DEVICE_NUM_LEDS);
  ms_anim_change.start(i_animation_duration); // Default animation time.

  // Change the addressable LED to black by default.
  fill_solid(device_leds, DEVICE_NUM_LEDS, CRGB::Black);

  // Prepare the on-board RGB LED to be used as an output pin for indication.
  digitalWrite(BUILT_IN_LED, LOW); // Turn off the built-in LED.

  // Delay before configuring and running tasks.
  delay(200);

  /**
   * By default the WiFi will run on core0, while the standard loop() runs on core1.
   * We can make efficient use of the available cores by "pinning" a task to a core.
   * The ESP32 platform comes with FreeRTOS implemented internally and exposed even
   * to the Arduino platform (meaning: no need for using the ESP-IDF exclusively).
   * In theory this allows for improved parallel processing with prioritization and
   * granting of dedicated memory stacks to each task (which can be monitored).
   *
   * Parameters:
   *  Task Function Name,
   *  User-Friendly Task Name,
   *  Stack Size (in bytes),
   *  Input Parameter,
   *  Priority (use higher #),
   *  Task Handle Reference,
   *  Pinned Core (0 or 1)
   */

  // Create a single-run setup task with the highest priority for WiFi/WebServer startup.
  xTaskCreatePinnedToCore(PreferencesTask, "PreferencesTask", 4096, NULL, 6, &PreferencesTaskHandle, 1);

  // Delay all lower priority tasks until Preferences are loaded.
  vTaskDelay(100 / portTICK_PERIOD_MS); // Delay for 100ms to avoid competition.

  // Create a single-run setup task with the highest priority for WiFi/WebServer startup.
  xTaskCreatePinnedToCore(WiFiSetupTask, "WiFiSetupTask", 4096, NULL, 5, &WiFiSetupTaskHandle, 1);

  // Delay all lower priority tasks until WiFi and WebServer setup is done.
  vTaskDelay(200 / portTICK_PERIOD_MS); // Delay for 200ms to avoid competition.

  // Create tasks which utilize a loop for continuous operation (prioritized highest to lowest).
  xTaskCreatePinnedToCore(UserInputTask, "UserInputTask", 4096, NULL, 3, &UserInputTaskHandle, 1);
  xTaskCreatePinnedToCore(AnimationTask, "AnimationTask", 4096, NULL, 2, &AnimationTaskHandle, 1);
  xTaskCreatePinnedToCore(WiFiManagementTask, "WiFiManagementTask", 4096, NULL, 1, &WiFiManagementTaskHandle, 0);

  // Create idle tasks for each core, used to estimate % busy for core.
  #if defined(DEBUG_PERFORMANCE)
  xTaskCreatePinnedToCore(idleTaskCore0, "Idle Task Core 0", 1000, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(idleTaskCore1, "Idle Task Core 1", 1000, NULL, 1, NULL, 1);
  #endif
}


void loop() {
  // No work done here, only in the tasks!

  #if defined(DEBUG_PERFORMANCE)
  Serial.println(F("=================================================="));
  printCPULoad();      // Print CPU load
  printMemoryStats();  // Print memory usage
  delay(3000);         // Wait 5 seconds before printing again
  #endif

  // Exception: Run the WebSocket client loop if connected to WiFi.
  if (b_ext_wifi_started && b_socket_ready) {
    webSocket.loop();
  }
}