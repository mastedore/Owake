# Beverly

### Button Events for AVR Easily.

Beverly is a lightweight Arduino library for handling push buttons with debounce,
long press detection and clean state transitions.

It is designed primarily for AVR microcontrollers, but works on any Arduino-compatible
architecture that provides `digitalRead()` and `millis()`.

## Features

- Software debounce
- Press and release detection
- Long press detection (`BAction::Held`)
- Simple state-based API
- Minimal memory footprint
- Uses bitmask-based internal state (via Facebits)

## Dependencies

- [Facebits](https://github.com/markusianito/Facebits)

## Installation
### Arduino IDE
- Download the library as a ZIP
- Open Arduino IDE
- Sketch → Include Library → Add .ZIP Library

### Arduino Library Manager
Search for **Facebits** and install it directly.

## Basic Usage

See examples/button.ino

# Notes

    start() must be called in setup before using watch()

    Held is reported continuously while the button remains pressed

    Timing is handled using millis() with 16-bit overflow-safe logic

## License

MIT License