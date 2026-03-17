#include "motion.h"

#include "state.h"
#include "utils.h"

void pushInterval(uint32_t ms) {
  cadenceState.intervalBuf[cadenceState.intervalWrite] = ms;
  cadenceState.intervalWrite = (cadenceState.intervalWrite + 1) % INTERVAL_BUF;

  if (cadenceState.intervalCount < INTERVAL_BUF) {
    cadenceState.intervalCount++;
  }
}

float averageIntervalMs() {
  if (cadenceState.intervalCount <= 0) {
    return 0.0f;
  }

  uint64_t sum = 0;
  for (int i = 0; i < cadenceState.intervalCount; ++i) {
    sum += cadenceState.intervalBuf[i];
  }

  return static_cast<float>(sum) / static_cast<float>(cadenceState.intervalCount);
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
  if (cadenceState.lastTickMs != 0 && nowMs > cadenceState.lastTickMs) {
    pushInterval(nowMs - cadenceState.lastTickMs);
  }

  cadenceState.lastTickMs = nowMs;
  cadenceState.tickCount++;

  cadenceState.cadencePerMin = computeCadencePerMin();
  motionState.currentImpulse = computeImpulse(cadenceState.cadencePerMin);

  // Each valid tick adds a forward impulse.
  motionState.currentSpeed = clampf_local(
    motionState.currentSpeed + motionState.currentImpulse,
    0.0f,
    1.0f
  );
}

void updateCadenceTimeout(uint32_t nowMs) {
  if (cadenceState.lastTickMs == 0) {
    cadenceState.cadencePerMin = 0.0f;
    motionState.currentImpulse = 0.0f;
    return;
  }

  if ((nowMs - cadenceState.lastTickMs) > CADENCE_TIMEOUT_MS) {
    cadenceState.cadencePerMin = 0.0f;
  }
}

void updateSpeedDecay(uint32_t nowMs) {
  const uint32_t deltaMs = nowMs - timingState.lastLoopMs;
  timingState.lastLoopMs = nowMs;

  if (deltaMs == 0) {
    return;
  }

  const float dt = static_cast<float>(deltaMs) / 1000.0f;

  if (cadenceState.lastTickMs == 0) {
    motionState.currentSpeed = 0.0f;
    motionState.currentImpulse = 0.0f;
    return;
  }

  const uint32_t sinceLastTickMs = nowMs - cadenceState.lastTickMs;
  const uint32_t stopTriggerMs = computeStopTriggerMs();
  const bool hardStopActive = (sinceLastTickMs > stopTriggerMs);

  if (hardStopActive) {
    motionState.currentImpulse = 0.0f;
    motionState.currentSpeed *= expf(-STOP_RATE * dt);

    if (motionState.currentSpeed < STOP_CUTOFF) {
      motionState.currentSpeed = 0.0f;
    }
  } else {
    motionState.currentSpeed *= expf(-DECAY * dt);

    if (motionState.currentSpeed < 0.0005f) {
      motionState.currentSpeed = 0.0f;
    }
  }
}