#pragma once

#include "config.h"

void pushInterval(uint32_t ms);
float averageIntervalMs();
float computeCadencePerMin();
float computeImpulse(float cadence);
int speedToAxisY(float speed);
uint32_t computeStopTriggerMs();
void registerTick(uint32_t nowMs);
void updateCadenceTimeout(uint32_t nowMs);
void updateSpeedDecay(uint32_t nowMs);