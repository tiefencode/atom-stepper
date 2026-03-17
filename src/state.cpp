#include "state.h"

BleGamepad bleGamepad("StepVR ATOM Stepper", "tiefencode", 100);
CRGB leds[LED_COUNT];

SensorState sensorState;
CadenceState cadenceState;
MotionState motionState;
TimingState timingState;
LedState ledState;