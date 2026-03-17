#include "led_ui.h"

#include "state.h"
#include "utils.h"

static void applyLedRgb(uint8_t r, uint8_t g, uint8_t b) {
  if (ledState.lastLedR == r &&
      ledState.lastLedG == g &&
      ledState.lastLedB == b) {
    return;
  }

  ledState.lastLedR = r;
  ledState.lastLedG = g;
  ledState.lastLedB = b;

  leds[0] = CRGB(r, g, b);
  FastLED.show();
}

void setupLed() {
  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, LED_COUNT);
  applyLedRgb(0, 0, 0);
}

void updateLed(uint32_t nowMs) {
  if ((nowMs - ledState.lastLedUpdateMs) < LED_REFRESH_MS) {
    return;
  }

  ledState.lastLedUpdateMs = nowMs;

  if (nowMs < ledState.ledStartupGreenUntilMs) {
    applyLedRgb(0, LED_STARTUP_GREEN_BRIGHTNESS, 0);
    return;
  }

  if (!bleGamepad.isConnected()) {
    if (nowMs >= ledState.ledRedEnableAtMs) {
      applyLedRgb(LED_DISCONNECTED_RED_BRIGHTNESS, 0, 0);
    } else {
      applyLedRgb(0, 0, 0);
    }
    return;
  }

  bool motionActive = false;
  if (cadenceState.lastTickMs != 0) {
    const uint32_t sinceLastTickMs = nowMs - cadenceState.lastTickMs;
    motionActive = (sinceLastTickMs <= LED_OFF_HOLD_MS);
  }

  if (!motionActive) {
    const uint32_t phaseMs = nowMs % LED_IDLE_HEARTBEAT_INTERVAL_MS;
    if (phaseMs < LED_IDLE_HEARTBEAT_ON_MS) {
      applyLedRgb(0, LED_IDLE_HEARTBEAT_GREEN_BRIGHTNESS, 0);
    } else {
      applyLedRgb(0, 0, 0);
    }
    return;
  }

  // Pulse rate is driven directly by the current speed.
  const float speed = (motionState.currentSpeed > 0.0001f) ? motionState.currentSpeed : 0.0001f;
  const uint32_t pulseMs = static_cast<uint32_t>(lroundf(LED_SPEED_DIVISOR / speed));
  const float pulse = pulse01(nowMs, pulseMs);
  const uint8_t blue = lerp8_local(
    LED_MOVE_BLUE_DIM_BRIGHTNESS,
    LED_MOVE_BLUE_BRIGHT_BRIGHTNESS,
    pulse
  );

  applyLedRgb(0, 0, blue);
}