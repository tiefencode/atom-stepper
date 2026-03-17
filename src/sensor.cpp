#include "sensor.h"

#include "motion.h"
#include "state.h"

void updateSensor(uint32_t nowMs) {
  const int rawState = digitalRead(HALL_PIN);

  if (rawState != sensorState.lastRawState) {
    sensorState.lastRawState = rawState;
    sensorState.lastRawChangeMs = nowMs;
  }

  if ((nowMs - sensorState.lastRawChangeMs) < DEBOUNCE_MS) {
    return;
  }

  if (rawState != sensorState.stableState) {
    sensorState.stableState = rawState;
    sensorState.stableSinceMs = nowMs;

    // Count one tick on HIGH -> LOW, but only after the sensor
    // stayed HIGH long enough to re-arm.
    if (sensorState.stableState == LOW) {
      if (sensorState.sensorArmed) {
        registerTick(nowMs);
        sensorState.sensorArmed = false;
      }
    }
  }

  if (sensorState.stableState == HIGH &&
      !sensorState.sensorArmed &&
      (nowMs - sensorState.stableSinceMs) >= ARM_HIGH_MS) {
    sensorState.sensorArmed = true;
  }
}