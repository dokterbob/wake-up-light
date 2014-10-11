/* NeoPixel-based wake up light */

#include <math.h>
#include "neopixel/neopixel.h"

// Delay between steps, in milliseconds
#define DELAY 100

#define RED 255
#define GREEN 253
#define BLUE 134

// EEPROM/Flash allocation of hour and minute
#define HOUR_ADDRESS 0
#define MINUTE_ADDRESS 1
#define DURATION_ADDRESS 2

#define HOUR_DEFAULT 7
#define MINUTE_DEFAULT 20
#define DURATION_DEFAULT 1800000 // Half an hour

// RAM versions of hour and minute
int hour, minute, duration;

#define ONE_DAY_MILLIS (24 * 60 * 60 * 1000)
unsigned long lastSync = millis();

// Power indicator
int led = D7;
int button = D2;

unsigned long lastFade = millis();
unsigned long startFade;
bool fading = false;
volatile bool do_reset = false;

// NeoPixel properties
#define PIXEL_PIN D6
#define PIXEL_COUNT 38
#define PIXEL_TYPE WS2812B
Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);

void setup_api() {
    // Setup Spark API

    // Allow manual start/stop of fading
    Spark.function("start_fading", startFading);
    Spark.function("reset", reset);

    // Allow setting timer
    Spark.function("set_hour", setHour);
    Spark.function("set_minute", setMinute);
    // Shit! As opposed to the docs, the spark only takes 4 functions...
    // TODO: Make time take HH:MM
    // Spark.function("set_duration", setDuration);

    // Allow reading hour and minute
    Spark.variable("get_hour", &hour, INT);
    Spark.variable("get_minute", &minute, INT);
    Spark.variable("get_duration", &duration, INT);
    Spark.variable("is_fading", &fading, INT);
}

void setup_strip() {
    // Initialize all pixels to 'off'
    strip.begin();
    strip.show();
}

void setup_settings() {
    // Get hour and minute from EEPROM
    hour = EEPROM.read(HOUR_ADDRESS);
    minute = EEPROM.read(MINUTE_ADDRESS);
    duration = EEPROM.read(DURATION_ADDRESS);

    // Sensible defaults
    if (hour == 0) {
        hour = HOUR_DEFAULT;
    }

    if (minute == 0) {
        minute = MINUTE_DEFAULT;
    }

    // if (duration == 0) {
        duration = DURATION_DEFAULT;
    // }
}

void setup() {
    // Serial over USB for debugging
    Serial.begin(9600);

    setup_api();
    setup_strip();
    setup_settings();

    // Timezone CEST
    Time.zone(2);

    // Indicator LED (is_fading)
    pinMode(led, OUTPUT);
    digitalWrite(led, LOW);

    // Reset button
    pinMode(button, INPUT_PULLDOWN);
    attachInterrupt(button, request_reset, RISING);

    Serial.println("Initialized");
}

void loop() {
    if (!fading && Time.hour() == hour && Time.minute() == minute) {
        // Start fading
        startFading(NULL);
    }

    if (do_reset) {
        // Reset requested by interrupt
        reset(NULL);
        do_reset = false;
    }

    fadeIn();

    // Sync time once a day
    syncTime();
}

int setHour(String arg) {
    // Set wakeup hour

    char converted = arg.toInt();

    EEPROM.write(HOUR_ADDRESS, converted);
    hour = converted;

    return converted;
}

int setMinute(String arg) {
    // Set wakeup hour

    char converted = arg.toInt();

    EEPROM.write(MINUTE_ADDRESS, converted);
    minute = converted;

    return converted;
}

int setDuration(String arg) {
    // Set duration

    char converted = arg.toInt();

    EEPROM.write(DURATION_ADDRESS, converted);
    duration = converted;

    return converted;
}

int startFading(String arg) {
    // Enable fading (unless already enabled)
    if (!fading) {
        Serial.println("Enabling fading");

        fading = true;
        startFade = millis();

        digitalWrite(led, HIGH);

        // Success
        return 1;
    }

    return -1;
}

void request_reset() {
    // Interrupt for requesting reset
    do_reset = true;
}

int reset(String arg) {
    // Disable fading (unless already disabled)
    Serial.println("Reset fading");

    fading = false;

    // Switch off LED's
    strip.setBrightness(0);
    strip.show();

    digitalWrite(led, LOW);

    // Success
    return 1;
}

void syncTime() {
    if (millis() - lastSync > ONE_DAY_MILLIS) {
        // Request time synchronization from the Spark Cloud
        Spark.syncTime();
        lastSync = millis();
    }
}

void setStripColor(uint32_t c) {
    // Set colour for entire strip

    char i;

    for(i=0; i<strip.numPixels(); i++) {
        strip.setPixelColor(i, c);
    }

}

double square_easing(double x) {
    // Easing function in range x: [0, 1] -> [0, 1]
    return x*x;
}

double power_easing(double x, double p) {
    // Easying with random power
    return pow(x, p);
}

char floattochar(float x) {
    // Convert float [0, 1] -> char [0, 254]
    return (char)floor(x*254);
}

uint32_t blackBodyColor(float temp) {
    // Return colour based on temperature
    float x = temp / 1000.0;
    float x2 = x * x;
    float x3 = x2 * x;
    float x4 = x3 * x;
    float x5 = x4 * x;

    float R, G, B = 0.0;

    // red
    if (temp <= 6600.0)
        R = 1.0;
    else
        R = 0.0002889 * x5 - 0.01258 * x4 + 0.2148 * x3 - 1.776 * x2 + 6.907 * x - 8.723;

    // green
    if (temp <= 6600.0)
        G = -4.593e-05 * x5 + 0.001424 * x4 - 0.01489 * x3 + 0.0498 * x2 + 0.1669 * x - 0.1653;
    else
        G = -1.308e-07 * x5 + 1.745e-05 * x4 - 0.0009116 * x3 + 0.02348 * x2 - 0.3048 * x + 2.159;

    // blue
    if (temp <= 2000.0)
        B = 0.0;
    else if (temp < 6600.0)
        B = 1.764e-05 * x5 + 0.0003575 * x4 - 0.01554 * x3 + 0.1549 * x2 - 0.3682 * x + 0.2386;
    else
        B = 1.0;

    return strip.Color(floattochar(R), floattochar(G), floattochar(B));
}

void fadeIn() {
    // Fade in NeoPixels

    float phase;
    unsigned long cur_millis = millis();

    if (fading && cur_millis - lastFade > DELAY) {
        phase = float(cur_millis - startFade) / duration;

        if (phase >= 1.0) {
            // Stop fading
            Serial.println("Done fading");

            fading = false;
        }

        // Change colour temp from 3500 to 10000
        setStripColor(blackBodyColor(3500.0 + power_easing(phase, 3)*6500.0));
        strip.setBrightness(floattochar(square_easing(phase)));
        strip.show();

        lastFade = cur_millis;
    }

}
