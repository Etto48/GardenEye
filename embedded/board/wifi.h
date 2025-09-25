#pragma once


namespace gardeneye::wifi {
    // Switch on WiFi and try to connect
    // Returns false if failed to connect
    bool init();

    // Disconnect and switch off WiFi to save power
    void stop();

    // Get the MAC address of the WiFi interface as a string
    // The returned pointer is to a static buffer
    char* get_mac_address();
}