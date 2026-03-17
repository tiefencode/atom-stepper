#include "ble_output.h"

#include "motion.h"
#include "state.h"

void sendGamepad(uint32_t nowMs) {
  if (!bleGamepad.isConnected()) {
    return;
  }

  if ((nowMs - timingState.lastBleUpdateMs) < BLE_UPDATE_MS) {
    return;
  }

  timingState.lastBleUpdateMs = nowMs;

  const int x = AXIS_CENTER;
  const int y = speedToAxisY(motionState.currentSpeed);

  // Keep all other axes centered/zeroed.
  bleGamepad.setAxes(x, y, 0, 0, 0, 0, 0, 0);
}