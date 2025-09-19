#include <WiFi.h>
#include <HTTPClient.h>
#include <esp_wifi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "private.h"

constexpr float MAX_V = 3.3; // max voltage for ADC, equivalent to MAX_ANALOG reading from analogRead(...)
constexpr float DIVIDER_CORRECTIVE_FACTOR = 3; // corrective factor for voltage divider
constexpr uint16_t ANALOG_ACCURACY = 10; // number of analog reads to average
constexpr uint16_t MAX_ANALOG = 4095; // max analog read value for 12 bit ADC
constexpr uint64_t uS_TO_S_FACTOR = 1000000; /* Conversion factor for micro seconds to seconds */

constexpr uint8_t HUMIDITY_PIN = 35; // GPIO to read humidity
constexpr uint8_t TEMPERATURE_PIN = 33; // GPIO to read temperature
constexpr uint8_t BATTERY_PIN = 32; // GPIO to read battery voltage
constexpr uint8_t SENSOR_POWER_PIN = 13; // GPIO to power the sensors

constexpr uint64_t MAX_SENSOR_READINGS = 24; // max number of readings to store in RTC memory
constexpr uint64_t SAMPLING_INTERVAL_S = 1; // sample every N seconds
constexpr uint64_t SENDING_INTERVAL_SAMPLES = 1; // send every N samples 
constexpr uint64_t MAX_CONNECTION_TIME_S = 30; // max time to wait for wifi connection
constexpr uint64_t HTTP_TIMEOUT_S = 10; // timeout for HTTP operationsl

constexpr uint64_t MAX_JSON_SIZE = 1024*4; // max size of JSON payload
constexpr uint64_t MAX_FIELD_LENGTH = 32; // max length for individual field in JSON

struct SensorReadings {
    uint64_t timestamp;
    float humidity;
    float temperature;
    float battery;
};

RTC_DATA_ATTR struct SensorReadings data[MAX_SENSOR_READINGS];
RTC_DATA_ATTR uint64_t sensor_readings_start = 0;
RTC_DATA_ATTR uint64_t sensor_readings_len = 0;
RTC_DATA_ATTR uint64_t base_time = 0;

OneWire one_wire(TEMPERATURE_PIN);
DallasTemperature temperature_sensors(&one_wire);

bool wifi_init() {
    WiFi.mode(WIFI_MODE_STA);
    // WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
    WiFi.setHostname("GardenEye");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    // Add timeout to prevent infinite hanging
    int timeout_count = 0;
    const int MAX_WIFI_TIMEOUT = MAX_CONNECTION_TIME_S * 10; // *10 to go from 1s to 100ms 
    
    while (WiFi.status() != WL_CONNECTED && timeout_count < MAX_WIFI_TIMEOUT) {
        Serial.print(".");
        delay(100);
        timeout_count++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.print("\nESP32 IP Address: ");
        Serial.println(WiFi.localIP());
        Serial.print("ESP32 HostName: ");
        Serial.println(WiFi.getHostname());
        Serial.print("RRSI: ");
        Serial.println(WiFi.RSSI());
        return true;
    } else {
        Serial.println("\nWiFi connection failed - timeout reached");
        return false;
    }
}

void wifi_sleep() {
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
}

uint64_t get_current_timestamp() {
    return (esp_timer_get_time() / 1000 + base_time) / 1000;
}

void update_base_time() {
    // base_time += SAMPLING_INTERVAL_S * 1000; // TODO: uncomment for production
}

struct SensorReadings read_sensors() {
    struct SensorReadings ret = {
        .timestamp = get_current_timestamp(),
        .humidity = 0,
        .temperature = 0,
        .battery = 0,
    };

    double h = 0;
    double b = 0;    
    digitalWrite(SENSOR_POWER_PIN, HIGH);
    delay(20);
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
    while (!temperature_sensors.isConversionComplete()) {
        delay(10);
    }
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

SensorReadings* iter_reading(uint64_t *index) {
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

char *format_battery(float battery) {
    static char buf[MAX_FIELD_LENGTH];
    snprintf(buf, MAX_FIELD_LENGTH, "%f", battery * MAX_V * DIVIDER_CORRECTIVE_FACTOR);
    return buf;
}

char *format_humidity(float humidity) {
    static char buf[MAX_FIELD_LENGTH];
    snprintf(buf, MAX_FIELD_LENGTH, "%f", (1-humidity) * 100);
    return buf;
}

char *format_temperature(float temperature) {
    static char buf[MAX_FIELD_LENGTH];
    snprintf(buf, MAX_FIELD_LENGTH, "%f", temperature);
    return buf;
}

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
    while((reading = iter_reading(&index)) != nullptr) {
        float temp_value = (reading->temperature * MAX_V - 0.1 * MAX_V) * 100;
        written = snprintf(json + pos, MAX_JSON_SIZE - pos, "%f", temp_value);
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
    while((reading = iter_reading(&index)) != nullptr) {
        float humidity_value = (1 - reading->humidity) * 100;
        written = snprintf(json + pos, MAX_JSON_SIZE - pos, "%f", humidity_value);
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
    while((reading = iter_reading(&index)) != nullptr) {
        float battery_value = reading->battery * MAX_V * DIVIDER_CORRECTIVE_FACTOR;
        written = snprintf(json + pos, MAX_JSON_SIZE - pos, "%f", battery_value);
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
    while((reading = iter_reading(&index)) != nullptr) {
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
    written = snprintf(json + pos, MAX_JSON_SIZE - pos, "],\"now\":%llu}", get_current_timestamp());
    if (written >= MAX_JSON_SIZE - pos) {
        Serial.println("JSON buffer overflow at closing");
        return nullptr;
    }
    pos += written;

    return json;
}

bool upload_readings() {
    // -1 for error
    HTTPClient client;
    uint8_t baseMac[6];
    char mac_str[18];

    esp_wifi_get_mac(WIFI_IF_STA, baseMac);
    snprintf(mac_str, 18, "%02x:%02x:%02x:%02x:%02x:%02x", baseMac[0], baseMac[1], baseMac[2], baseMac[3], baseMac[4], baseMac[5]);
    client.begin("http://ettorex1.local:8000/api/readings");
    
    // Set timeout to prevent hanging
    client.setTimeout(HTTP_TIMEOUT_S * 1000); // *1000 to convert to ms
    
    client.addHeader("MAC", mac_str);
    client.addHeader("Authorization", "Bearer " API_KEY);
    client.addHeader("Content-Type", "application/json");
    
    char* json_payload = serialize_readings_to_json_stack();
    if (json_payload == nullptr) {
        Serial.println("Failed to serialize JSON - buffer overflow");
        client.end();
        return false;
    }
    
    int code = client.POST((uint8_t *)json_payload, strlen(json_payload));
    if (code > 0) {
        Serial.printf("HTTP POST code: %d\n", code);
        String response = client.getString();
        Serial.println(response);
        if (code >= 200 && code < 300) {
            clear_readings();
        }
    } else {
        Serial.printf("POST failed, error: %s\n", client.errorToString(code).c_str());
    }
    
    client.end(); // Free resources
    if (code >= 200 && code < 300) {
        return true;
    } else {
        return false;
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(SENSOR_POWER_PIN, OUTPUT);
    // Power off the wireless module
    wifi_sleep();
    
    // Take a reading
    auto reading = read_sensors();
    push_reading(reading);
    Serial.printf("Battery: %sV, Temperature: %sC, Humidity: %s%%, Readings: %llu/%llu\n", 
        format_battery(reading.battery), 
        format_temperature(reading.temperature), 
        format_humidity(reading.humidity), 
        sensor_readings_len, 
        SENDING_INTERVAL_SAMPLES);

    // If we have enough readings, upload them
    if (sensor_readings_len >= SENDING_INTERVAL_SAMPLES) {
        if (wifi_init()) {
            upload_readings();
        }
        wifi_sleep();
    }
    // Go to sleep
    update_base_time();
    esp_sleep_enable_timer_wakeup(SAMPLING_INTERVAL_S * uS_TO_S_FACTOR);
    esp_deep_sleep_start();
}

void loop() {
    // Should never reach here in normal operation
    Serial.println("Unexpected loop entry, going back to sleep");
    esp_sleep_enable_timer_wakeup(SAMPLING_INTERVAL_S * uS_TO_S_FACTOR);
    esp_deep_sleep_start();
}
