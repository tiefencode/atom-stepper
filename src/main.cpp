#include "config.h"

#include "battery.h"
#include "ble_output.h"
#include "led_ui.h"
#include "motion.h"
#include "sensor.h"
#include "state.h"

void setup() {
  setCpuFrequencyMhz(CPU_FREQUENCY_MHZ);

  pinMode(HALL_PIN, INPUT);
  setupLed();
  setupBattery();

  const uint32_t nowMs = millis();
  sensorState.lastRawState = digitalRead(HALL_PIN);
  sensorState.stableState = sensorState.lastRawState;
  sensorState.lastRawChangeMs = nowMs;
  sensorState.stableSinceMs = nowMs;
  timingState.lastLoopMs = nowMs;
  ledState.ledStartupGreenUntilMs = nowMs + LED_STARTUP_GREEN_DURATION_MS;
  ledState.ledRedEnableAtMs = nowMs + LED_RED_ENABLE_DELAY_MS;

  // Only arm immediately if the sensor is idle-high at boot.
  sensorState.sensorArmed = (sensorState.stableState == HIGH);

  bleGamepad.configuration.setTXPowerLevel(BLE_TX_POWER_DBM);
  bleGamepad.begin(&bleGamepad.configuration);
}

void loop() {
  const uint32_t nowMs = millis();

  updateSpeedDecay(nowMs);
  updateSensor(nowMs);
  updateCadenceTimeout(nowMs);
  updateBattery(nowMs);
  updateLed(nowMs);
  sendGamepad(nowMs);

  delay(MAIN_LOOP_DELAY_MS);
}
