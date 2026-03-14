# StepVR ATOM Stepper

Firmware for an M5Stack ATOM Lite / ESP32 that turns stepper ticks from a Hall sensor into a forward BLE gamepad axis.

## Hardware

* M5Stack ATOM Lite
* Hall sensor on `GPIO 32`
* Internal RGB LED on `GPIO 27`

## Features

* Tick-based forward impulse model
* Exponential decay between ticks
* Dynamic hard stop when expected ticks stop
* BLE gamepad output
* LED status feedback

## Project layout

```
include/
  config.h
  state.h
  utils.h
  sensor.h
  motion.h
  led_ui.h
  ble_output.h

src/
  main.cpp
  state.cpp
  sensor.cpp
  motion.cpp
  led_ui.cpp
  ble_output.cpp
```

## Build

Example `platformio.ini`:

```
[env:m5stack-atom]
platform = espressif32
board = m5stack-atom
framework = arduino

monitor_speed = 115200

lib_deps =
  lemmingdev/ESP32-BLE-Gamepad @ ^0.7.3
  h2zero/NimBLE-Arduino @ ^2.2.1
  jasonlzt/FastLED
```

## Runtime model

* Hall sensor input is debounced and re-armed on HIGH
* A valid `HIGH -> LOW` transition registers one tick
* Each tick adds a forward impulse
* Speed decays exponentially between ticks
* A stronger stop engages when the next expected tick is missed
* Forward motion is sent as BLE gamepad Y axis

## LED states

* **Green**: short startup indication after boot/flash
* **Red**: BLE not connected, delayed briefly after boot
* **Blue**: motion active, pulse rate driven directly by `currentSpeed`
* **Dim green blink**: connected idle heartbeat
* **Off**: connected idle outside heartbeat window

## Main tuning values

### Motion feel

* `BASE`
* `GAIN`
* `CMIN`
* `CMAX`
* `IGAMMA`
* `DECAY`

### Hard stop

* `STOP_TRIGGER_FACTOR`
* `STOP_MIN_MS`
* `STOP_MAX_MS`
* `STOP_RATE`
* `STOP_CUTOFF`

### LED

* `LED_SPEED_DIVISOR`
* `LED_OFF_HOLD_MS`
* `LED_MOVE_BLUE_DIM`
* `LED_MOVE_BLUE_BRIGHT`

## Notes

* `LED_SPEED_DIVISOR` is the main LED pulse tuning value
* Higher `LED_SPEED_DIVISOR` = slower blue pulsing
* Current split keeps the existing behavior and prepares the codebase for further cleanup
