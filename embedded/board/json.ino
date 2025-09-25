#include <ArduinoJson.h>

#include "json.h"
#include "types.h"
#include "sensors.h"

namespace gardeneye::json {
    char *serialize_readings_to_json_stack() {
        static char json[MAX_JSON_SIZE];
        
        // Start building JSON
        int pos = 0;
        int written = 0;
        
        written = snprintf(json + pos, MAX_JSON_SIZE - pos, "{\"temperature\":[");
        if (written >= MAX_JSON_SIZE - pos) {
            Serial.println("JSON buffer overflow at temperature start");
            return nullptr;
        }
        pos += written;
        
        // Add temperature values
        struct SensorReadings* reading = nullptr;
        uint64_t index = 0;
        while((reading = gardeneye::sensors::iter_reading(&index)) != nullptr) {
            written = snprintf(json + pos, MAX_JSON_SIZE - pos, "%f", gardeneye::sensors::temperature_value(reading->temperature));
            if (written >= MAX_JSON_SIZE - pos) {
                Serial.println("JSON buffer overflow at temperature values");
                return nullptr;
            }
            pos += written;
            
            if (index < sensor_readings_len) {
                written = snprintf(json + pos, MAX_JSON_SIZE - pos, ",");
                if (written >= MAX_JSON_SIZE - pos) {
                    Serial.println("JSON buffer overflow at temperature comma");
                    return nullptr;
                }
                pos += written;
            }
        }
        
        written = snprintf(json + pos, MAX_JSON_SIZE - pos, "],\"humidity\":[");
        if (written >= MAX_JSON_SIZE - pos) {
            Serial.println("JSON buffer overflow at humidity start");
            return nullptr;
        }
        pos += written;
        
        // Add humidity values
        index = 0;
        while((reading = gardeneye::sensors::iter_reading(&index)) != nullptr) {
            written = snprintf(json + pos, MAX_JSON_SIZE - pos, "%f", gardeneye::sensors::humidity_value(reading->humidity));
            if (written >= MAX_JSON_SIZE - pos) {
                Serial.println("JSON buffer overflow at humidity values");
                return nullptr;
            }
            pos += written;
            
            if (index < sensor_readings_len) {
                written = snprintf(json + pos, MAX_JSON_SIZE - pos, ",");
                if (written >= MAX_JSON_SIZE - pos) {
                    Serial.println("JSON buffer overflow at humidity comma");
                    return nullptr;
                }
                pos += written;
            }
        }
        
        written = snprintf(json + pos, MAX_JSON_SIZE - pos, "],\"battery\":[");
        if (written >= MAX_JSON_SIZE - pos) {
            Serial.println("JSON buffer overflow at battery start");
            return nullptr;
        }
        pos += written;
        
        // Add battery values
        index = 0;
        while((reading = gardeneye::sensors::iter_reading(&index)) != nullptr) {
            written = snprintf(json + pos, MAX_JSON_SIZE - pos, "%f", gardeneye::sensors::battery_value(reading->battery));
            if (written >= MAX_JSON_SIZE - pos) {
                Serial.println("JSON buffer overflow at battery values");
                return nullptr;
            }
            pos += written;
            
            if (index < sensor_readings_len) {
                written = snprintf(json + pos, MAX_JSON_SIZE - pos, ",");
                if (written >= MAX_JSON_SIZE - pos) {
                    Serial.println("JSON buffer overflow at battery comma");
                    return nullptr;
                }
                pos += written;
            }
        }
        
        written = snprintf(json + pos, MAX_JSON_SIZE - pos, "],\"timestamps\":[");
        if (written >= MAX_JSON_SIZE - pos) {
            Serial.println("JSON buffer overflow at timestamps start");
            return nullptr;
        }
        pos += written;
        
        // Add timestamp values
        index = 0;
        while((reading = gardeneye::sensors::iter_reading(&index)) != nullptr) {
            written = snprintf(json + pos, MAX_JSON_SIZE - pos, "%llu", reading->timestamp);
            if (written >= MAX_JSON_SIZE - pos) {
                Serial.println("JSON buffer overflow at timestamp values");
                return nullptr;
            }
            pos += written;
            
            if (index < sensor_readings_len) {
                written = snprintf(json + pos, MAX_JSON_SIZE - pos, ",");
                if (written >= MAX_JSON_SIZE - pos) {
                    Serial.println("JSON buffer overflow at timestamp comma");
                    return nullptr;
                }
                pos += written;
            }
        }
        
        // Close timestamps array and add current timestamp
        written = snprintf(json + pos, MAX_JSON_SIZE - pos, "],\"now\":%llu}", gardeneye::timing::get_current_timestamp());
        if (written >= MAX_JSON_SIZE - pos) {
            Serial.println("JSON buffer overflow at closing");
            return nullptr;
        }
        pos += written;

        return json;
    }   

    bool sync_base_time_from_string(String json) {
        JsonDocument doc;
        auto error = deserializeJson(doc, json);
        if (error != DeserializationError::Ok) {
            Serial.printf("Failed to parse time sync JSON: %s\n", error.c_str());
            return false;
        }
        if (!doc["base_time"].is<uint64_t>() || !doc["next_sync"].is<uint64_t>()) {
            Serial.println("Invalid time sync JSON: missing keys");
            return false;
        }
        gardeneye::timing::set_current_time(doc["base_time"].as<uint64_t>());
        next_sync = doc["next_sync"].as<uint64_t>();
        Serial.printf("Time synced\n");
        return true;
    }
}