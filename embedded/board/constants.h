#pragma once

constexpr float MAX_V = 3.3; // max voltage for ADC, equivalent to MAX_ANALOG reading from analogRead(...)
constexpr float DIVIDER_CORRECTIVE_FACTOR = 2; // corrective factor for voltage divider
constexpr uint16_t ANALOG_ACCURACY = 10; // number of analog reads to average
constexpr uint16_t MAX_ANALOG = 4095; // max analog read value for 12 bit ADC
constexpr uint64_t uS_TO_S_FACTOR = 1000000; /* Conversion factor for micro seconds to seconds */

constexpr uint8_t HUMIDITY_PIN = 35; // GPIO to read humidity
constexpr uint8_t TEMPERATURE_PIN = 33; // GPIO to read temperature
constexpr uint8_t BATTERY_PIN = 32; // GPIO to read battery voltage
constexpr uint8_t SENSOR_POWER_PIN = 13; // GPIO to power the sensors

constexpr uint64_t MAX_SENSOR_READINGS = 24; // max number of readings to store in RTC memory
constexpr uint64_t SAMPLING_INTERVAL_S = 10; // sample every N seconds
constexpr uint64_t MIN_SYNC_SAMPLES = 12; // send every N samples 
constexpr uint64_t MAX_CONNECTION_TIME_S = 30; // max time to wait for wifi connection
constexpr uint64_t HTTP_TIMEOUT_S = 10; // timeout for HTTP operations
constexpr uint64_t RETRY_SETUP_S = 10; // wait time before retrying first time setup

constexpr uint64_t MAX_JSON_SIZE = 1024 * 4; // max size of JSON payload
constexpr uint64_t MAX_FIELD_LENGTH = 32; // max length for individual field in JSON

constexpr float CRITICAL_V = 3.0; // critical battery voltage

#define SERVER_HOSTNAME "gardeneye.local"
#define SERVER_URL "http://" SERVER_HOSTNAME
#define READINGS_URL SERVER_URL "/api/readings"
#define TIME_URL SERVER_URL "/api/time"
#define REGISTER_URL SERVER_URL "/api/register"