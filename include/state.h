#pragma once

#include "config.h"

struct SensorState {
  int lastRawState = HIGH;
  int stableState = HIGH;
  uint32_t lastRawChangeMs = 0;
  uint32_t stableSinceMs = 0;
  bool sensorArmed = false;
};

struct CadenceState {
  uint32_t tickCount = 0;
  uint32_t lastTickMs = 0;
  uint32_t intervalBuf[INTERVAL_BUF] = {};
  int intervalCount = 0;
  int intervalWrite = 0;
  float cadencePerMin = 0.0f;
};

struct MotionState {
  float currentImpulse = 0.0f;
  float currentSpeed = 0.0f;
};

struct TimingState {
  uint32_t lastBleUpdateMs = 0;
  uint32_t lastPrintMs = 0;
  uint32_t lastLoopMs = 0;
};

struct LedState {
  uint32_t ledStartupGreenUntilMs = 0;
  uint32_t ledRedEnableAtMs = 0;
  uint32_t lastLedUpdateMs = 0;
  uint8_t lastLedR = 255;
  uint8_t lastLedG = 255;
  uint8_t lastLedB = 255;
  float ledPulsePhase = 0.0f;
  uint32_t lastLedPhaseStepMs = 0;
};

extern BleGamepad bleGamepad;
extern CRGB leds[LED_COUNT];

extern SensorState sensorState;
extern CadenceState cadenceState;
extern MotionState motionState;
extern TimingState timingState;
extern LedState ledState;
