#include "integrity.h"

namespace gardeneye::integrity {
    void update_battery_status(struct SensorReadings reading) {
        auto battery = gardeneye::sensors::battery_value(reading.battery);
        if (battery <= CRITICAL_V && battery >= 0.01) { // If battery is disconnected don't stop
            critical_battery_detected = true;
        }
    }

    void check_if_enough_battery() {
        if (critical_battery_detected) {
            esp_deep_sleep_start(); // Enter deep sleep forever
        }
    }
    
    // This function is called only on the very first boot, not after deep sleep
    // It's used to sync time and register to the server
    // Returns true when it skips or succeeds 
    bool first_time_setup() {
        
        bool success = true;
        if (base_time == 0) { // If time was never synced, so this function have never succeeded
            Serial.println("First time setup - syncing time");
            if (gardeneye::wifi::init()) {
                success = success && gardeneye::rest::register_device();
                success = success && gardeneye::rest::sync_base_time();
                gardeneye::wifi::stop();
            } else {
                Serial.println("Failed to connect to WiFi for first time setup");
                success = false;
            }
        }
        return success;
    }
}