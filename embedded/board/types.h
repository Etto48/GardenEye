#pragma once

// Structure to hold sensor readings
struct SensorReadings {
    uint64_t timestamp;
    float humidity;
    float temperature;
    float battery;
};