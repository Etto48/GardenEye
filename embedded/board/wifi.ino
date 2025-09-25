#include <WiFi.h>
#include <esp_wifi.h>

#include "wifi.h"

namespace gardeneye::wifi {

    bool init() {

        WiFi.mode(WIFI_MODE_STA);
        // WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
        WiFi.setHostname("gardeneye-sensor");
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        
        // Add timeout to prevent infinite hanging
        int timeout_count = 0;
        const int MAX_WIFI_TIMEOUT = MAX_CONNECTION_TIME_S * 10; // *10 to go from 1s to 100ms 
        
        while (WiFi.status() != WL_CONNECTED && timeout_count < MAX_WIFI_TIMEOUT) {
            Serial.print(".");
            delay(100);
            timeout_count++;
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            Serial.print("\nESP32 IP Address: ");
            Serial.println(WiFi.localIP());
            Serial.print("ESP32 HostName: ");
            Serial.println(WiFi.getHostname());
            Serial.print("RRSI: ");
            Serial.println(WiFi.RSSI());
            return true;
        } else {
            Serial.println("\nWiFi connection failed - timeout reached");
            return false;
        }
    }

    void stop() {
        WiFi.disconnect();
        WiFi.mode(WIFI_OFF);
    }

    char* get_mac_address() {
        static char mac_str[18] = {0};
        uint8_t baseMac[6];
        if (mac_str[0] != 0) {
            return mac_str; // Already computed
        }
        esp_wifi_get_mac(WIFI_IF_STA, baseMac);
        snprintf(mac_str, 18, "%02x:%02x:%02x:%02x:%02x:%02x", baseMac[0], baseMac[1], baseMac[2], baseMac[3], baseMac[4], baseMac[5]);
        return mac_str;
    }
}