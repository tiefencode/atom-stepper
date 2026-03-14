#include "sensor.h"

#include "motion.h"
#include "state.h"

void updateSensor(uint32_t nowMs) {
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