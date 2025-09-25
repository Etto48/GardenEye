#pragma once

extern struct SensorReadings data[MAX_SENSOR_READINGS]; // Circular buffer for sensor readings
extern uint64_t sensor_readings_start; // Start index in the circular buffer
extern uint64_t sensor_readings_len; // Number of readings in the circular buffer
extern uint64_t base_time; // base time in seconds since epoch
extern uint64_t next_sync; // next recommended sync time in seconds since epoch
extern bool critical_battery_detected; // True if critical battery level was detected
