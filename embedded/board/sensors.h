#pragma once
#include "types.h"

namespace gardeneye::sensors {
    // Convert the raw battery reading to voltage
    float battery_value(float battery);

    // Convert the raw humidity reading to percentage
    float humidity_value(float humidity);

    // Convert the raw temperature reading to celsius
    float temperature_value(float temperature);

    // Power on the sensors, read them, power them off and return the readings
    struct SensorReadings read_sensors();

    // Push a new reading to the circular buffer
    void push_reading(struct SensorReadings reading);

    // Iterate over the readings in the circular buffer
    // Returns nullptr when done
    // You need to pass a uint64_t index initialized to 0 that will be updated
    // to keep track of the iteration
    struct SensorReadings* iter_reading(uint64_t *index);

    // Clear all readings from the circular buffer
    void clear_readings();

    // Check if there are enough readings to upload
    bool needs_upload();
}
