#include "led_ui.h"

#include "state.h"
#include "utils.h"

static float wrapPhase01(float phase) {
  while (phase >= 1.0f) {
    phase -= 1.0f;
  }

  while (phase < 0.0f) {
    phase += 1.0f;
  }

  return phase;
}

static float pulseFromPhase(float phase) {
  return 0.5f - (0.5f * cosf(wrapPhase01(phase) * 2.0f * PI));
}

static float advanceLedPulsePhase(uint32_t nowMs, float speed) {
  const float clampedSpeed = (speed > 0.0001f) ? speed : 0.0001f;
  const float pulseMs = LED_SPEED_DIVISOR / clampedSpeed;

  if (ledState.lastLedPhaseStepMs == 0) {
    ledState.lastLedPhaseStepMs = nowMs;
    return ledState.ledPulsePhase;
  }

  const uint32_t deltaMs = nowMs - ledState.lastLedPhaseStepMs;
  ledState.lastLedPhaseStepMs = nowMs;

  if (pulseMs <= 1.0f) {
    ledState.ledPulsePhase = 0.0f;
    return ledState.ledPulsePhase;
  }

  ledState.ledPulsePhase = wrapPhase01(
    ledState.ledPulsePhase + (static_cast<float>(deltaMs) / pulseMs)
  );

  return ledState.ledPulsePhase;
}

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
    ledState.ledPulsePhase = 0.0f;
    ledState.lastLedPhaseStepMs = nowMs;

    const uint32_t phaseMs = nowMs % LED_IDLE_HEARTBEAT_INTERVAL_MS;
    if (phaseMs < LED_IDLE_HEARTBEAT_ON_MS) {
      applyLedRgb(0, LED_IDLE_HEARTBEAT_GREEN_BRIGHTNESS, 0);
    } else {
      applyLedRgb(0, 0, 0);
    }
    return;
  }

  // Pulse rate is driven directly by the current speed.
  const float pulse = pulseFromPhase(
    advanceLedPulsePhase(nowMs, motionState.currentSpeed)
  );
  const uint8_t blue = lerp8_local(
    LED_MOVE_BLUE_DIM_BRIGHTNESS,
    LED_MOVE_BLUE_BRIGHT_BRIGHTNESS,
    pulse
  );

  applyLedRgb(0, 0, blue);
}
