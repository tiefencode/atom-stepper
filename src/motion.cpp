#include "motion.h"

#include "state.h"
#include "utils.h"

void pushInterval(uint32_t ms) {
  intervalBuf[intervalWrite] = ms;
  intervalWrite = (intervalWrite + 1) % INTERVAL_BUF;

  if (intervalCount < INTERVAL_BUF) {
    intervalCount++;
  }
}

float averageIntervalMs() {
  if (intervalCount <= 0) {
    return 0.0f;
  }

  uint64_t sum = 0;
  for (int i = 0; i < intervalCount; ++i) {
    sum += intervalBuf[i];
  }
  return static_cast<float>(sum) / static_cast<float>(intervalCount);
}

float computeCadencePerMin() {
  const float avgMs = averageIntervalMs();
  if (avgMs <= 0.0f) {
    return 0.0f;
  }
  return 60000.0f / avgMs;
}

float computeImpulse(float cadence) {
  if (cadence <= 0.0f) {
    return BASE;
  }

  const float denom = (CMAX - CMIN);
  if (denom <= 0.0f) {
    return clampf_local(BASE, 0.0f, 1.0f);
  }

  const float norm = clampf_local((cadence - CMIN) / denom, 0.0f, 1.0f);
  const float shaped = powf(norm, IGAMMA);
  return clampf_local(BASE + (GAIN * shaped), 0.0f, 1.0f);
}

int speedToAxisY(float speed) {
  const float clamped = clampf_local(speed, 0.0f, 1.0f);
  const int y = static_cast<int>(lroundf(static_cast<float>(AXIS_CENTER) - (clamped * AXIS_CENTER)));
  return clampi_local(y, AXIS_MIN, AXIS_MAX);
}

uint32_t computeStopTriggerMs() {
  float expectedMs = averageIntervalMs();

  if (expectedMs <= 0.0f) {
    expectedMs = static_cast<float>(CADENCE_TIMEOUT_MS);
  }

  const float triggerMs = clampf_local(
    expectedMs * STOP_TRIGGER_FACTOR,
    static_cast<float>(STOP_MIN_MS),
    static_cast<float>(STOP_MAX_MS)
  );

  return static_cast<uint32_t>(lroundf(triggerMs));
}

void registerTick(uint32_t nowMs) {
  if (lastTickMs != 0 && nowMs > lastTickMs) {
    pushInterval(nowMs - lastTickMs);
  }

  lastTickMs = nowMs;
  tickCount++;

  cadencePerMin = computeCadencePerMin();
  currentImpulse = computeImpulse(cadencePerMin);

  // Each valid tick adds a forward impulse.
  currentSpeed = clampf_local(currentSpeed + currentImpulse, 0.0f, 1.0f);
}

void updateCadenceTimeout(uint32_t nowMs) {
  if (lastTickMs == 0) {
    cadencePerMin = 0.0f;
    currentImpulse = 0.0f;
    return;
  }

  if ((nowMs - lastTickMs) > CADENCE_TIMEOUT_MS) {
    cadencePerMin = 0.0f;
  }
}

void updateSpeedDecay(uint32_t nowMs) {
  const uint32_t deltaMs = nowMs - lastLoopMs;
  lastLoopMs = nowMs;

  if (deltaMs == 0) {
    return;
  }

  const float dt = static_cast<float>(deltaMs) / 1000.0f;

  if (lastTickMs == 0) {
    currentSpeed = 0.0f;
    currentImpulse = 0.0f;
    return;
  }

  const uint32_t sinceLastTickMs = nowMs - lastTickMs;
  const uint32_t stopTriggerMs = computeStopTriggerMs();
  const bool hardStopActive = (sinceLastTickMs > stopTriggerMs);

  if (hardStopActive) {
    currentImpulse = 0.0f;
    currentSpeed *= expf(-STOP_RATE * dt);

    if (currentSpeed < STOP_CUTOFF) {
      currentSpeed = 0.0f;
    }
  } else {
    currentSpeed *= expf(-DECAY * dt);

    if (currentSpeed < 0.0005f) {
      currentSpeed = 0.0f;
    }
  }
}