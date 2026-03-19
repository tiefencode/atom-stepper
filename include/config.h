#pragma once

#include <Arduino.h>
#include <BleGamepad.h>
#include <FastLED.h>
#include <math.h>

static const int HALL_PIN = 32;
static const int BATTERY_ADC_PIN = 33;
static const int LED_PIN = 27;
static const int LED_COUNT = 1;

// Sensor filtering
static const uint32_t DEBOUNCE_MS = 12;
static const uint32_t ARM_HIGH_MS = 45;

// Timing
static const uint32_t BLE_UPDATE_MS = 20;
static const uint32_t CADENCE_TIMEOUT_MS = 1500;
static const uint32_t MAIN_LOOP_DELAY_MS = 4;
static const int8_t BLE_TX_POWER_DBM = 0;
static const uint32_t BATTERY_SAMPLE_MS = 2000;
static const uint32_t BATTERY_NOTIFY_MS = 15000;
static const uint8_t BATTERY_SAMPLE_COUNT = 8;

// CPU
static const uint32_t CPU_FREQUENCY_MHZ = 80;

// Cadence smoothing
static const int INTERVAL_BUF = 4;

// Impulse model
static const float BASE = 0.25f;      // low-speed floor; lower = less movement on very slow steps
static const float GAIN = 0.40f;      // top-end boost; higher = more speed when pushing harder
static const float CMIN = 40.0f;      // boost starts here; higher = less low-speed sensitivity
static const float CMAX = 80.0f;      // full boost reached here; lower = top-end comes earlier
static const float IGAMMA = 1.20f;    // curve shape; higher = calmer low end, stronger high end

// Coast behavior
static const float DECAY = 1.20f;     // higher = less glide between ticks

// Hard stop behavior
static const float STOP_TRIGGER_FACTOR = 1.4f; // lower = stop logic starts earlier after missed step
static const uint32_t STOP_MIN_MS = 480;        // minimum delay before hard stop can engage
static const uint32_t STOP_MAX_MS = 1400;       // maximum delay before hard stop must engage
static const float STOP_RATE = 50.0f;           // higher = harder stop once stop logic is active
static const float STOP_CUTOFF = 0.080f;        // snap to zero below this speed during hard stop

// Axis range for BLE Gamepad
static const int AXIS_MIN = 0;
static const int AXIS_CENTER = 16384;
static const int AXIS_MAX = 32767;

// LED behavior
static const uint32_t LED_STARTUP_GREEN_DURATION_MS = 1000;       // show green briefly after boot/flash
static const uint8_t LED_STARTUP_GREEN_BRIGHTNESS = 20;           // startup green brightness
static const uint32_t LED_RED_ENABLE_DELAY_MS = 3000;             // do not show red immediately after boot
static const uint8_t LED_DISCONNECTED_RED_BRIGHTNESS = 6;         // BLE not connected
static const uint32_t LED_REFRESH_MS = 40;                        // throttle LED updates to reduce power + bus activity
static const uint32_t LED_OFF_HOLD_MS = 2200;                     // only turn LED fully off after real inactivity
static const float LED_SPEED_DIVISOR = 14000.0f;                  // single tuning value for speed -> pulse period
static const uint8_t LED_MOVE_BLUE_DIM_BRIGHTNESS = 1;            // minimum blue while motion is active
static const uint8_t LED_MOVE_BLUE_BRIGHT_BRIGHTNESS = 8;         // maximum blue while motion is active
static const uint32_t LED_IDLE_HEARTBEAT_INTERVAL_MS = 10000;     // connected idle heartbeat interval
static const uint32_t LED_IDLE_HEARTBEAT_ON_MS = 40;              // connected idle heartbeat on-time
static const uint8_t LED_IDLE_HEARTBEAT_GREEN_BRIGHTNESS = 1;     // connected idle heartbeat brightness
