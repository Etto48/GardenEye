#include "private.h"
#include "types.h"
#include "json.h"
#include "sensors.h"
#include "wifi.h"
#include "constants.h"
#include "rest.h"
#include "rtc_data.h"
#include "timing.h"
#include "integrity.h"

void setup() {
    Serial.begin(115200);
    pinMode(SENSOR_POWER_PIN, OUTPUT);
    pinMode(LED_PIN, OUTPUT);

    digitalWrite(LED_PIN, HIGH);

    if (!gardeneye::integrity::first_time_setup()) {
        Serial.printf("First time setup failed, retrying in %llus\n", RETRY_SETUP_S);
        digitalWrite(LED_PIN, LOW);
        esp_sleep_enable_timer_wakeup(RETRY_SETUP_S * uS_TO_S_FACTOR);
        esp_deep_sleep_start();
    }

    // Power off the wireless module
    gardeneye::wifi::stop();

    // Take a reading
    auto reading = gardeneye::sensors::read_sensors();
    gardeneye::sensors::push_reading(reading);
    Serial.printf("Battery: %fV, Temperature: %fC, Humidity: %f%%, Readings: %llu/%llu\n", 
        gardeneye::sensors::battery_value(reading.battery), 
        gardeneye::sensors::temperature_value(reading.temperature), 
        gardeneye::sensors::humidity_value(reading.humidity), 
        sensor_readings_len, 
        MIN_SYNC_SAMPLES);

    gardeneye::integrity::update_battery_status(reading);
    // gardeneye::integrity::check_if_enough_battery();

    digitalWrite(LED_PIN, LOW);
    delay(100);
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(100);
    digitalWrite(LED_PIN, HIGH);
    delay(100);

    // If we have enough readings, or it's time to sync, upload them
    if (gardeneye::timing::needs_sync() || gardeneye::sensors::needs_upload()) {
        if (gardeneye::wifi::init()) {
            gardeneye::rest::upload_readings();
            gardeneye::rest::sync_base_time();
        }
        gardeneye::wifi::stop();
    }
    // Go to sleep
    gardeneye::timing::update_base_time();
    digitalWrite(LED_PIN, LOW);
    esp_sleep_enable_timer_wakeup(SAMPLING_INTERVAL_S * uS_TO_S_FACTOR);
    esp_deep_sleep_start();
}

void loop() {
    // In case the code returns from setup
    esp_sleep_enable_timer_wakeup(SAMPLING_INTERVAL_S * uS_TO_S_FACTOR);
    esp_deep_sleep_start();
}
