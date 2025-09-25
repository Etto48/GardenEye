#pragma once

namespace gardeneye::json {
    // Serialize the readings to a JSON string
    // The JSON format is:
    // ```json
    // {
    //   "temperature": [<temp1>, <temp2>, ...],
    //   "humidity": [<hum1>, <hum2>, ...],
    //   "battery": [<bat1>, <bat2>, ...],
    //   "timestamps": [<ts1>, <ts2>, ...],
    //   "now": <current_time>
    // }
    // ```
    // Returns a pointer to a static buffer, or nullptr on failure
    // The function uses only stack memory
    char *serialize_readings_to_json_stack();

    // Sync the base time from a JSON string in the format:
    // ```json
    // {
    //   "base_time": <base_time>,
    //   "next_sync": <next_sync>
    // }
    // ```
    // Returns true on success, false on failure
    bool sync_base_time_from_string(String json);
}