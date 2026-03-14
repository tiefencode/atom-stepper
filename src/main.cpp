#include "config.h"

#include "ble_output.h"
#include "led_ui.h"
#include "motion.h"
#include "sensor.h"
#include "state.h"

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
  Serial.println("BUILD: impulse-decay-v3-split");
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