#include <WiFi.h>
#include <HTTPClient.h>

#include "rest.h"
#include "wifi.h"
#include "rtc_data.h"
#include "json.h"
#include "sensors.h"

namespace gardeneye::rest {
    bool sync_base_time() {
        HTTPClient client;
        client.begin(TIME_URL);
        client.setTimeout(HTTP_TIMEOUT_S * 1000);
        int code = client.GET();
        bool failed = false;
        if (code >= 200 && code < 300) {
            // Success
            String response = client.getString();
            if (!gardeneye::json::sync_base_time_from_string(response)) {
                failed = true;
            }
        } else {
            Serial.printf("Time sync failed, error: %s\n", client.errorToString(code).c_str());
            failed = true;
        }
        if (failed) {
            next_sync = 0; // try again next time
        }
        return !failed;
    }

    bool register_device() {
        HTTPClient client;
        auto mac_str = gardeneye::wifi::get_mac_address();
        client.begin(REGISTER_URL);
        client.setTimeout(HTTP_TIMEOUT_S * 1000);
        
        client.addHeader("MAC", mac_str);
        client.addHeader("Authorization", "Bearer " API_KEY);

        int code = client.POST(nullptr, 0);
        if (code >= 200 && code < 300) {
            Serial.printf("Device registered\n");
            return true;
        } else {
            Serial.printf("Device registration failed, error: %s\n", client.errorToString(code).c_str());
            return false;
        }
    }

    bool upload_readings() {
        // False for error
        HTTPClient client;
        auto mac_str = gardeneye::wifi::get_mac_address();
        client.begin(READINGS_URL);
        
        // Set timeout to prevent hanging
        client.setTimeout(HTTP_TIMEOUT_S * 1000); // *1000 to convert to ms
        
        client.addHeader("MAC", mac_str);
        client.addHeader("Authorization", "Bearer " API_KEY);
        client.addHeader("Content-Type", "application/json");
        
        char* json_payload = gardeneye::json::serialize_readings_to_json_stack();
        if (json_payload == nullptr) {
            Serial.println("Failed to serialize JSON - buffer overflow");
            client.end();
            return false;
        }
        
        int code = client.POST((uint8_t *)json_payload, strlen(json_payload));
        if (code > 0) {
            Serial.printf("HTTP POST code: %d\n", code);
            String response = client.getString();
            Serial.println(response);
            if (code >= 200 && code < 300) {
                gardeneye::sensors::clear_readings();
            }
        } else {
            Serial.printf("POST failed, error: %s\n", client.errorToString(code).c_str());
        }
        
        client.end(); // Free resources
        if (code >= 200 && code < 300) {
            return true;
        } else {
            return false;
        }
    }
}