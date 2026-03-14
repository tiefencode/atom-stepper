#pragma once

#include "config.h"

static inline float clampf_local(float v, float lo, float hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

static inline int clampi_local(int v, int lo, int hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

static inline uint8_t lerp8_local(uint8_t a, uint8_t b, float t) {
  const float clamped = clampf_local(t, 0.0f, 1.0f);
  return static_cast<uint8_t>(lroundf(a + (b - a) * clamped));
}

static inline float pulse01(uint32_t nowMs, uint32_t periodMs) {
  if (periodMs < 2) {
    return 1.0f;
  }

  const float phase = static_cast<float>(nowMs % periodMs) / static_cast<float>(periodMs);
  return 0.5f - (0.5f * cosf(phase * 2.0f * PI));
}