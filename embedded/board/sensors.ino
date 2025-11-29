#include <OneWire.h>
#include <DallasTemperature.h>

#include "sensors.h"
#include "constants.h"
#include "rtc_data.h"

namespace gardeneye::sensors {
    OneWire one_wire(TEMPERATURE_PIN);
    DallasTemperature temperature_sensors(&one_wire);

    float battery_value(float battery) {
        return battery * MAX_V * BATTERY_SCALING_M + BATTERY_SCALING_Q;
    }

    float humidity_value(float humidity) {
        return (1 - humidity) * 100;
    }

    float temperature_value(float temperature) {
        return temperature;
    }

    struct SensorReadings read_sensors() {
        struct SensorReadings ret = {
            .timestamp = gardeneye::timing::get_current_timestamp(),
            .humidity = 0,
            .temperature = 0,
            .battery = 0,
        };

        double h = 0;
        double b = 0;    
        digitalWrite(SENSOR_POWER_PIN, HIGH);
        delay(20);
        temperature_sensors.begin();
        temperature_sensors.setWaitForConversion(false);
        temperature_sensors.setResolution(12);
        temperature_sensors.requestTemperatures();
        for (uint64_t i = 0; i < ANALOG_ACCURACY; i++) 
        {
            h += double(analogRead(HUMIDITY_PIN))/MAX_ANALOG;
            b += double(analogRead(BATTERY_PIN))/MAX_ANALOG;
            delay(10);
        }
        ret.humidity = h/ANALOG_ACCURACY;
        ret.battery = b/ANALOG_ACCURACY;
        temperature_sensors.blockTillConversionComplete(0);
        ret.temperature = temperature_sensors.getTempCByIndex(0);

        digitalWrite(SENSOR_POWER_PIN, LOW);
        return ret;
    }

    void push_reading(struct SensorReadings reading) {
        uint64_t index = (sensor_readings_start + sensor_readings_len) % MAX_SENSOR_READINGS;
        data[index] = reading;
        if (sensor_readings_len < MAX_SENSOR_READINGS) {
            sensor_readings_len++;
        } else {
            sensor_readings_start = (sensor_readings_start + 1) % MAX_SENSOR_READINGS;
        }
    }

    struct SensorReadings* iter_reading(uint64_t *index) {
        if (*index >= sensor_readings_len) {
            return nullptr;
        }
        uint64_t real_index = (sensor_readings_start + *index) % MAX_SENSOR_READINGS;
        (*index)++;
        return &data[real_index];
    }

    void clear_readings() {
        sensor_readings_start = 0;
        sensor_readings_len = 0;
    }

    bool needs_upload() {
        return sensor_readings_len >= MIN_SYNC_SAMPLES;
    }
}