#include "battery.h"

#include "state.h"
#include "utils.h"

namespace {

uint8_t batteryPercentFromMillivolts(uint16_t batteryMv) {
  if (batteryMv <= 3000) {
    return 0;
  }
  if (batteryMv >= 4200) {
    return 100;
  }

  if (batteryMv <= 3470) {
    return static_cast<uint8_t>((batteryMv - 3000) * 25UL / (3470 - 3000));
  }
  if (batteryMv <= 3610) {
    return static_cast<uint8_t>(25 + ((batteryMv - 3470) * 25UL / (3610 - 3470)));
  }
  if (batteryMv <= 3810) {
    return static_cast<uint8_t>(50 + ((batteryMv - 3610) * 25UL / (3810 - 3610)));
  }

  return static_cast<uint8_t>(75 + ((batteryMv - 3810) * 25UL / (4200 - 3810)));
}

void sampleBattery() {
  const uint16_t batteryMv = static_cast<uint16_t>(analogReadMilliVolts(BATTERY_ADC_PIN) * 2U);

  batteryState.sampleSumMv += batteryMv;
  batteryState.sampleCount += 1;

  if (batteryState.sampleCount < BATTERY_SAMPLE_COUNT) {
    return;
  }

  batteryState.batteryMv = static_cast<uint16_t>(batteryState.sampleSumMv / batteryState.sampleCount);
  batteryState.batteryPercent = batteryPercentFromMillivolts(batteryState.batteryMv);
  batteryState.sampleSumMv = 0;
  batteryState.sampleCount = 0;
}

}  // namespace

void setupBattery() {
  pinMode(BATTERY_ADC_PIN, INPUT);
  analogReadResolution(12);
  analogSetPinAttenuation(BATTERY_ADC_PIN, ADC_11db);

  for (uint8_t i = 0; i < BATTERY_SAMPLE_COUNT; ++i) {
    sampleBattery();
    delay(2);
  }

  bleGamepad.setBatteryLevel(batteryState.batteryPercent);
  batteryState.lastSentPercent = batteryState.batteryPercent;
}

void updateBattery(uint32_t nowMs) {
  if ((nowMs - batteryState.lastSampleMs) >= BATTERY_SAMPLE_MS) {
    batteryState.lastSampleMs = nowMs;
    sampleBattery();
  }

  const bool percentChanged = batteryState.batteryPercent != batteryState.lastSentPercent;
  const bool notifyDue = (nowMs - batteryState.lastNotifyMs) >= BATTERY_NOTIFY_MS;

  if (!percentChanged && !notifyDue) {
    return;
  }

  bleGamepad.setBatteryLevel(batteryState.batteryPercent);
  batteryState.lastSentPercent = batteryState.batteryPercent;
  batteryState.lastNotifyMs = nowMs;
}
