#include "rtc_data.h"
#include "types.h"

RTC_DATA_ATTR struct SensorReadings data[MAX_SENSOR_READINGS];
RTC_DATA_ATTR uint64_t sensor_readings_start = 0;
RTC_DATA_ATTR uint64_t sensor_readings_len = 0;
RTC_DATA_ATTR uint64_t base_time = 0; // base time in seconds since epoch
RTC_DATA_ATTR uint64_t next_sync = 0; // next recommended sync time in seconds since epoch
RTC_DATA_ATTR bool critical_battery_detected = false;