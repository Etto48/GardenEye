#pragma once

namespace gardeneye::rest {
    // Sync the base time from the server
    // Returns true on success, false on failure
    // This function must be called with WiFi connected
    bool sync_base_time();

    // Register the device to the server
    // Returns true on success, false on failure
    // This function must be called with WiFi connected
    bool register_device();

    // Upload the readings to the server
    // Returns true on success, false on failure
    // This function must be called with WiFi connected
    // On success, the readings are cleared from the buffer
    // On failure, the readings are kept for later retry
    bool upload_readings();
}