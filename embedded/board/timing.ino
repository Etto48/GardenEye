#include "timing.h"

namespace gardeneye::timing {    
    uint64_t get_current_timestamp() {
        return (esp_timer_get_time() / 1000000 + base_time);
    }
    
    void update_base_time() {
        base_time += SAMPLING_INTERVAL_S;
    }

    void set_current_time(uint64_t current_time) {
        base_time = current_time - (esp_timer_get_time() / 1000000);
    }

    bool needs_sync() {
        return next_sync != 0 && get_current_timestamp() >= next_sync;
    }
}