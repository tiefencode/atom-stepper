#pragma once

#include "config.h"

extern BleGamepad bleGamepad;
extern CRGB leds[LED_COUNT];

// Sensor state
extern int lastRawState;
extern int stableState;
extern uint32_t lastRawChangeMs;
extern uint32_t stableSinceMs;
extern bool sensorArmed;

// Tick + cadence state
extern uint32_t tickCount;
extern uint32_t lastTickMs;
extern uint32_t intervalBuf[INTERVAL_BUF];
extern int intervalCount;
extern int intervalWrite;
extern float cadencePerMin;

// Motion state
extern float currentImpulse;
extern float currentSpeed;

// Timing state
extern uint32_t lastBleUpdateMs;
extern uint32_t lastPrintMs;
extern uint32_t lastLoopMs;

// LED state
extern uint32_t startupGreenUntilMs;
extern uint32_t redEnableAtMs;
extern uint32_t lastLedUpdateMs;
extern uint8_t lastLedR;
extern uint8_t lastLedG;
extern uint8_t lastLedB;