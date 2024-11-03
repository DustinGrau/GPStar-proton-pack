/**
 *   gpstar External - Ghostbusters Proton Pack & Neutrona Wand.
 *   Copyright (C) 2023-2024 Michael Rajotte <michael.rajotte@gpstartechnologies.com>
 *                         & Dustin Grau <dustin.grau@gmail.com>
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

/**
 * Wireless (WiFi) Communications for ESP32
 *
 * This device will use the SoftAP mode to act as a standalone WiFi access point, allowing
 * direct connections to the device without need for a full wireless network. All address
 * (IP) assignments will be handled as part of the code here.
 *
 * Note that per the Expressif programming guide: "ESP32 has only one 2.4 GHz ISM band RF
 * module, which is shared by Bluetooth (BT & BLE) and Wi-Fi, so Bluetooth can’t receive
 * or transmit data while Wi-Fi is receiving or transmitting data and vice versa. Under
 * such circumstances, ESP32 uses the time-division multiplexing method to receive and
 * transmit packets."
 *
 * Essentially performance suffers when both WiFi and Bluetooth are enabled and so we
 * must choose which is more useful to the operation of this device. Decision: WiFi.
 *
 * https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/coexist.html
 */
#include <Preferences.h>
#include <WiFi.h>
#include <WiFiAP.h>
#include <ESPmDNS.h>
#include <AsyncJson.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>
#include <WebSocketsClient.h>

// Preferences for SSID and AP password, which will use a "credentials" namespace.
Preferences preferences;

// Set up values for the SSID and password for the WiFi access point (AP).
const String ap_default_ssid = "ProtonPack_D418"; // This will be the base of the SSID name.
String ap_default_passwd = "12345678"; // This will be the default password for the AP.
String ap_ssid; // Reserved for storing the true SSID for the AP to be set at startup.
String ap_pass; // Reserved for storing the true AP password set by the user.

// Prepare variables for client connections
millisDelay ms_wifiretry;
const unsigned int i_wifi_retry_wait = 1000; // How long between attempts to find the wifi network
const unsigned int i_websocket_retry_wait = 3000; // How long between restoring the websocket
bool b_wifi_connected = false; // Denotes connection to expected wifi network
bool b_socket_config = false; // Denotes websocket configuration was performed

WebSocketsClient webSocket; // WebSocket client class instance

JsonDocument jsonDoc; // Used for processing JSON body data.

boolean startWiFi() {
  // Begin some diagnostic information to console.
  Serial.println();
  Serial.println("Starting Wireless Client");
  String macAddr = String(WiFi.macAddress());
  Serial.print("Device WiFi MAC Address: ");
  Serial.println(macAddr);

  // Prepare to return either stored preferences or a default value for SSID/password.
  preferences.begin("credentials", true); // Access namespace in read-only mode.
  ap_ssid = preferences.getString("ssid", ap_default_ssid);
  ap_pass = preferences.getString("password", ap_default_passwd);
  preferences.end();

  Serial.print("WiFi Network: ");
  Serial.println(ap_ssid);
  Serial.print("WiFi Password: ");
  Serial.println(ap_pass);

  // Start the access point using the SSID and password.
  return WiFi.begin(ap_ssid.c_str(), ap_pass.c_str());
}

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length) {
  if (type == WStype_CONNECTED) {
    Serial.println("WebSocket Connected");
    digitalWrite(BUILT_IN_LED, HIGH);
  }

  if (type == WStype_DISCONNECTED) {
    Serial.println("WebSocket Disconnected");
    digitalWrite(BUILT_IN_LED, LOW);
  }

  if (type == WStype_TEXT) {
    /*
     * Deserialize incoming JSON String from remote websocket server.
     * NOTE: Some data from the Attenuator/Wireless may be plain text
     * which will cause an error to be thrown. Only continue when no
     * error is present from deserialization.
     */
    DeserializationError jsonError = deserializeJson(jsonDoc, payload); 
    if (!jsonError) {
      // Store values as a known datatype (String).
      String data_mode = jsonDoc["mode"];
      String data_theme = jsonDoc["theme"];
      String data_switch = jsonDoc["switch"];
      String data_pack = jsonDoc["pack"];
      String data_safety = jsonDoc["safety"];
      String data_wand = jsonDoc["wand"];
      String data_wandMode = jsonDoc["wandMode"];
      String data_firing = jsonDoc["firing"];
      String data_cable = jsonDoc["cable"];
      String data_ctron = jsonDoc["cyclotron"];
      String data_temp = jsonDoc["temperature"];

      // Convert power (1-5) to an integer.
      i_power = (int)jsonDoc["power"];

      // Output some data to the serial console when needed.
      #if defined(USE_DEBUGS)
        Serial.print(data_wandMode);
        Serial.print(" is ");
        Serial.print(data_firing);
        Serial.print(" at level ");
        Serial.println(i_power);
      #endif

      // Change LED for testing
      if(data_firing == "Firing") {
        //Serial.println(data_firing);
        b_firing = true;

        if(data_wandMode == "Proton") {
          FIRING_MODE = PROTON;
        }
        else if(data_wandMode == "Slime") {
          FIRING_MODE = SLIME;
        }
        else if(data_wandMode == "Stasis") {
          FIRING_MODE = STASIS;
        }
        else if(data_wandMode == "Meson") {
          FIRING_MODE = MESON;
        }
        else if(data_wandMode == "Venting") {
          FIRING_MODE = VENTING;
        }
        else if(data_wandMode == "Settings") {
          FIRING_MODE = SETTINGS;
        }
        else {
          FIRING_MODE = SPECTRAL;
        }
      }
      else {
        //Serial.println(data_firing);
        b_firing = false;
      }
    }
  }
}