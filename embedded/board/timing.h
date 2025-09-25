#pragma once

namespace gardeneye::timing {
    // Get the current timestamp in seconds since epoch
    uint64_t get_current_timestamp();

    //  Update the base time by adding the sampling interval
    //  Should be called before going to sleep
    void update_base_time();

    // Set the current time, adjusting the base time accordingly
    void set_current_time(uint64_t current_time);

    // Check if it's time to sync
    bool needs_sync();
}