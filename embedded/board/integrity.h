#pragma once
#include "types.h"

namespace gardeneye::integrity {
    // Update the battery status based on the latest reading
    // If a critical battery level is detected, it sets a flag
    void update_battery_status(struct SensorReadings reading);

    // If a critical battery level was detected, enter deep sleep forever
    // to prevent battery damage
    void check_if_enough_battery();
 
    // This function is called only on the very first boot, not after deep sleep
    // It's used to sync time and register to the server
    // Returns true when it skips or succeeds 
    bool first_time_setup();
}