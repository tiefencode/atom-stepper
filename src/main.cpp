#include <Arduino.h>
#include <BleGamepad.h>
#include <FastLED.h>
#include <math.h>

static const int HALL_PIN = 32;
static const int LED_PIN = 27;
static const int LED_COUNT = 1;

// Sensor filtering
static const uint32_t DEBOUNCE_MS = 12;
static const uint32_t ARM_HIGH_MS = 45;

// Timing
static const uint32_t BLE_UPDATE_MS = 10;
static const uint32_t PRINT_MS = 200;
static const uint32_t CADENCE_TIMEOUT_MS = 1500;

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
static const uint32_t STARTUP_GREEN_DURATION_MS = 1000;   // show green briefly after boot/flash
static const uint32_t RED_ENABLE_DELAY_MS = 3000;         // do not show red immediately after boot
static const uint8_t STARTUP_GREEN_BRIGHTNESS = 20;       // startup green brightness
static const uint8_t LED_DISCONNECTED_RED = 6;            // BLE not connected
static const uint32_t LED_REFRESH_MS = 40;                // throttle LED updates to reduce power + bus activity
static const uint32_t LED_OFF_HOLD_MS = 2200;             // only turn LED fully off after real inactivity
static const float LED_SPEED_DIVISOR = 9000.0f;           // single tuning value for speed -> pulse period
static const uint8_t LED_MOVE_BLUE_DIM = 1;               // minimum blue while motion is active
static const uint8_t LED_MOVE_BLUE_BRIGHT = 8;            // maximum blue while motion is active
static const uint32_t IDLE_HEARTBEAT_INTERVAL_MS = 10000; // connected idle heartbeat interval
static const uint32_t IDLE_HEARTBEAT_ON_MS = 40;          // connected idle heartbeat on-time
static const uint8_t IDLE_HEARTBEAT_GREEN = 1;            // connected idle heartbeat brightness

BleGamepad bleGamepad("StepVR ATOM Stepper", "tiefencode", 100);
CRGB leds[LED_COUNT];

// Sensor state
int lastRawState = HIGH;
int stableState = HIGH;
uint32_t lastRawChangeMs = 0;
uint32_t stableSinceMs = 0;
bool sensorArmed = false;

// Tick + cadence state
uint32_t tickCount = 0;
uint32_t lastTickMs = 0;
uint32_t intervalBuf[INTERVAL_BUF] = {};
int intervalCount = 0;
int intervalWrite = 0;
float cadencePerMin = 0.0f;

// Motion state
float currentImpulse = 0.0f;
float currentSpeed = 0.0f;

// Timing state
uint32_t lastBleUpdateMs = 0;
uint32_t lastPrintMs = 0;
uint32_t lastLoopMs = 0;

// LED state
uint32_t startupGreenUntilMs = 0;
uint32_t redEnableAtMs = 0;
uint32_t lastLedUpdateMs = 0;
uint8_t lastLedR = 255;
uint8_t lastLedG = 255;
uint8_t lastLedB = 255;

static float clampf_local(float v, float lo, float hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

static int clampi_local(int v, int lo, int hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

static uint8_t lerp8_local(uint8_t a, uint8_t b, float t) {
  const float clamped = clampf_local(t, 0.0f, 1.0f);
  return static_cast<uint8_t>(lroundf(a + (b - a) * clamped));
}

static float pulse01(uint32_t nowMs, uint32_t periodMs) {
  if (periodMs < 2) {
    return 1.0f;
  }

  const float phase = static_cast<float>(nowMs % periodMs) / static_cast<float>(periodMs);
  return 0.5f - (0.5f * cosf(phase * 2.0f * PI));
}

static void applyLedRgb(uint8_t r, uint8_t g, uint8_t b) {
  if (lastLedR == r && lastLedG == g && lastLedB == b) {
    return;
  }

  lastLedR = r;
  lastLedG = g;
  lastLedB = b;

  leds[0] = CRGB(r, g, b);
  FastLED.show();
}

static void setupLed() {
  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, LED_COUNT);
  applyLedRgb(0, 0, 0);
}

static void pushInterval(uint32_t ms) {
  intervalBuf[intervalWrite] = ms;
  intervalWrite = (intervalWrite + 1) % INTERVAL_BUF;
  if (intervalCount < INTERVAL_BUF) {
    intervalCount++;
  }
}

static float averageIntervalMs() {
  if (intervalCount <= 0) {
    return 0.0f;
  }

  uint64_t sum = 0;
  for (int i = 0; i < intervalCount; ++i) {
    sum += intervalBuf[i];
  }
  return static_cast<float>(sum) / static_cast<float>(intervalCount);
}

static float computeCadencePerMin() {
  const float avgMs = averageIntervalMs();
  if (avgMs <= 0.0f) {
    return 0.0f;
  }
  return 60000.0f / avgMs;
}

static float computeImpulse(float cadence) {
  if (cadence <= 0.0f) {
    return BASE;
  }

  const float denom = (CMAX - CMIN);
  if (denom <= 0.0f) {
    return clampf_local(BASE, 0.0f, 1.0f);
  }

  const float norm = clampf_local((cadence - CMIN) / denom, 0.0f, 1.0f);
  const float shaped = powf(norm, IGAMMA);
  return clampf_local(BASE + (GAIN * shaped), 0.0f, 1.0f);
}

static int speedToAxisY(float speed) {
  const float clamped = clampf_local(speed, 0.0f, 1.0f);
  const int y = static_cast<int>(lroundf(static_cast<float>(AXIS_CENTER) - (clamped * AXIS_CENTER)));
  return clampi_local(y, AXIS_MIN, AXIS_MAX);
}

static uint32_t computeStopTriggerMs() {
  float expectedMs = averageIntervalMs();

  if (expectedMs <= 0.0f) {
    expectedMs = static_cast<float>(CADENCE_TIMEOUT_MS);
  }

  const float triggerMs = clampf_local(
    expectedMs * STOP_TRIGGER_FACTOR,
    static_cast<float>(STOP_MIN_MS),
    static_cast<float>(STOP_MAX_MS)
  );

  return static_cast<uint32_t>(lroundf(triggerMs));
}

static void registerTick(uint32_t nowMs) {
  if (lastTickMs != 0 && nowMs > lastTickMs) {
    pushInterval(nowMs - lastTickMs);
  }

  lastTickMs = nowMs;
  tickCount++;

  cadencePerMin = computeCadencePerMin();
  currentImpulse = computeImpulse(cadencePerMin);

  // Each valid tick adds a forward impulse.
  currentSpeed = clampf_local(currentSpeed + currentImpulse, 0.0f, 1.0f);
}

static void updateSensor(uint32_t nowMs) {
  const int rawState = digitalRead(HALL_PIN);

  if (rawState != lastRawState) {
    lastRawState = rawState;
    lastRawChangeMs = nowMs;
  }

  if ((nowMs - lastRawChangeMs) < DEBOUNCE_MS) {
    return;
  }

  if (rawState != stableState) {
    stableState = rawState;
    stableSinceMs = nowMs;

    // Count one tick on HIGH -> LOW, but only after the sensor
    // stayed HIGH long enough to re-arm.
    if (stableState == LOW) {
      if (sensorArmed) {
        registerTick(nowMs);
        sensorArmed = false;
      }
    }
  }

  if (stableState == HIGH && !sensorArmed && (nowMs - stableSinceMs) >= ARM_HIGH_MS) {
    sensorArmed = true;
  }
}

static void updateCadenceTimeout(uint32_t nowMs) {
  if (lastTickMs == 0) {
    cadencePerMin = 0.0f;
    currentImpulse = 0.0f;
    return;
  }

  if ((nowMs - lastTickMs) > CADENCE_TIMEOUT_MS) {
    cadencePerMin = 0.0f;
  }
}

static void updateSpeedDecay(uint32_t nowMs) {
  const uint32_t deltaMs = nowMs - lastLoopMs;
  lastLoopMs = nowMs;

  if (deltaMs == 0) {
    return;
  }

  const float dt = static_cast<float>(deltaMs) / 1000.0f;

  if (lastTickMs == 0) {
    currentSpeed = 0.0f;
    currentImpulse = 0.0f;
    return;
  }

  const uint32_t sinceLastTickMs = nowMs - lastTickMs;
  const uint32_t stopTriggerMs = computeStopTriggerMs();
  const bool hardStopActive = (sinceLastTickMs > stopTriggerMs);

  if (hardStopActive) {
    currentImpulse = 0.0f;
    currentSpeed *= expf(-STOP_RATE * dt);

    if (currentSpeed < STOP_CUTOFF) {
      currentSpeed = 0.0f;
    }
  } else {
    currentSpeed *= expf(-DECAY * dt);

    if (currentSpeed < 0.0005f) {
      currentSpeed = 0.0f;
    }
  }
}

static void updateLed(uint32_t nowMs) {
  if ((nowMs - lastLedUpdateMs) < LED_REFRESH_MS) {
    return;
  }

  lastLedUpdateMs = nowMs;

  if (nowMs < startupGreenUntilMs) {
    applyLedRgb(0, STARTUP_GREEN_BRIGHTNESS, 0);
    return;
  }

  if (!bleGamepad.isConnected()) {
    if (nowMs >= redEnableAtMs) {
      applyLedRgb(LED_DISCONNECTED_RED, 0, 0);
    } else {
      applyLedRgb(0, 0, 0);
    }
    return;
  }

  bool motionActive = false;
  if (lastTickMs != 0) {
    const uint32_t sinceLastTickMs = nowMs - lastTickMs;
    motionActive = (sinceLastTickMs <= LED_OFF_HOLD_MS);
  }

  if (!motionActive) {
    const uint32_t phaseMs = nowMs % IDLE_HEARTBEAT_INTERVAL_MS;
    if (phaseMs < IDLE_HEARTBEAT_ON_MS) {
      applyLedRgb(0, IDLE_HEARTBEAT_GREEN, 0);
    } else {
      applyLedRgb(0, 0, 0);
    }
    return;
  }

  // Pulse rate is driven directly by the current speed.
  const float speed = (currentSpeed > 0.0001f) ? currentSpeed : 0.0001f;
  const uint32_t pulseMs = static_cast<uint32_t>(lroundf(LED_SPEED_DIVISOR / speed));
  const float pulse = pulse01(nowMs, pulseMs);
  const uint8_t blue = lerp8_local(LED_MOVE_BLUE_DIM, LED_MOVE_BLUE_BRIGHT, pulse);

  applyLedRgb(0, 0, blue);
}

static void sendGamepad(uint32_t nowMs) {
  if (!bleGamepad.isConnected()) {
    return;
  }

  if ((nowMs - lastBleUpdateMs) < BLE_UPDATE_MS) {
    return;
  }

  lastBleUpdateMs = nowMs;

  const int x = AXIS_CENTER;
  const int y = speedToAxisY(currentSpeed);

  // Keep all other axes centered/zeroed.
  bleGamepad.setAxes(x, y, 0, 0, 0, 0, 0, 0);
}

static void printDebug(uint32_t nowMs) {
  if ((nowMs - lastPrintMs) < PRINT_MS) {
    return;
  }

  lastPrintMs = nowMs;

  const int x = AXIS_CENTER;
  const int y = speedToAxisY(currentSpeed);

  Serial.printf(
    "conn=%s stable=%s ticks=%lu cadence=%.1f/min impulse=%.2f speed=%.2f x=%d y=%d intervals=%d\n",
    bleGamepad.isConnected() ? "yes" : "no",
    (stableState == LOW) ? "LOW" : "HIGH",
    static_cast<unsigned long>(tickCount),
    cadencePerMin,
    currentImpulse,
    currentSpeed,
    x,
    y,
    intervalCount
  );
}

void setup() {
  Serial.begin(115200);

  pinMode(HALL_PIN, INPUT);
  setupLed();

  const uint32_t nowMs = millis();
  lastRawState = digitalRead(HALL_PIN);
  stableState = lastRawState;
  lastRawChangeMs = nowMs;
  stableSinceMs = nowMs;
  lastLoopMs = nowMs;
  startupGreenUntilMs = nowMs + STARTUP_GREEN_DURATION_MS;
  redEnableAtMs = nowMs + RED_ENABLE_DELAY_MS;

  // Only arm immediately if the sensor is idle-high at boot.
  sensorArmed = (stableState == HIGH);

  Serial.println();
  Serial.println("BUILD: impulse-decay-v3");
  Serial.printf(
    "BASE=%.2f GAIN=%.2f CMIN=%.1f CMAX=%.1f IGAMMA=%.2f DECAY=%.2f STOP_F=%.2f STOP_MIN=%lu STOP_MAX=%lu STOP_RATE=%.2f STOP_CUT=%.3f BUF=%d BLE=%lu PRINT=%lu\n",
    BASE, GAIN, CMIN, CMAX, IGAMMA, DECAY,
    STOP_TRIGGER_FACTOR,
    static_cast<unsigned long>(STOP_MIN_MS),
    static_cast<unsigned long>(STOP_MAX_MS),
    STOP_RATE, STOP_CUTOFF,
    INTERVAL_BUF,
    static_cast<unsigned long>(BLE_UPDATE_MS),
    static_cast<unsigned long>(PRINT_MS)
  );
  Serial.println("Impulse per tick + exponential decay between ticks");
  Serial.println("Dynamic hard stop after the expected next step is missed");
  Serial.println("Starting BLE gamepad...");

  bleGamepad.begin();
}

void loop() {
  const uint32_t nowMs = millis();

  updateSpeedDecay(nowMs);
  updateSensor(nowMs);
  updateCadenceTimeout(nowMs);
  updateLed(nowMs);
  sendGamepad(nowMs);
  printDebug(nowMs);

  delay(1);
}