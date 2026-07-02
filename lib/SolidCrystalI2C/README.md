# SolidCrystalI²C

SolidCrystalI²C is a **clean, minimal, powerful** library for HD44780-compatible LCDs using the PCF8574 I/O expander.

This library was written with a strong focus on:
- simplicity
- low RAM and flash usage
- predictable behavior on AVR
- modern, maintainable code

No legacy APIs, no `String`, no unnecessary abstractions.

---

## Why SolidCrystalI2C?

Most existing LiquidCrystal_I2C-style libraries:
- are unmaintained
- rely on outdated Arduino APIs
- pull in unnecessary features (`Print`, `String`, `Printable`)
- mix unused or duplicated code
- are hard to extend or reason about

SolidCrystalI2C takes a different approach:
- written **from scratch**
- based directly on the HD44780 datasheet
- minimal I2C transactions
- explicit control over timing and signaling
- small, readable codebase

---

## Features

- HD44780-compatible LCD support
- I2C via PCF8574
- 4-bit mode
- Minimal RAM footprint
- No `String`
- No dependency on legacy LiquidCrystal APIs
- AVR-friendly, but works on other Arduino-supported architectures

---

## Requirements

- Arduino-compatible environment
- I2C-capable MCU
- LCD with HD44780 controller
- PCF8574-based I2C backpack

---

## Installation

### Arduino Library Manager
Once available in the Library Manager:
1. Open Arduino IDE
2. Go to **Sketch → Include Library → Manage Libraries**
3. Search for **SolidCrystalI2C**
4. Install

### Manual installation
1. Download or clone this repository
2. Place it inside your Arduino `libraries` folder
3. Restart the Arduino IDE

---

## Basic Usage

```cpp
#include <SolidCrystalI2C.h>

LCD lcd(0x27, 16, 2);

void setup() {
    lcd.start();
    lcd << "Hello World!";
    lcd.flush();

}

void loop() {
}
```

## Design Philosophy

SolidCrystalI2C is intentionally small and explicit.

- Each byte sent over I2C maps directly to PCF8574 pins
- LCD data is written in 4-bit mode using precise EN pulses
- No hidden delays or background logic
- Behavior is deterministic and easy to reason about

The goal is not feature count, but control and clarity.

## Architecture Overview

    Low-level PCF8574 write routines

    Explicit nibble-based LCD communication

    Clear separation between command and data writes

    Minimal public API

This makes the library easy to extend or integrate into larger projects.
License

This project is licensed under the MIT License.
See the LICENSE file for details.
Author


## Notes

SolidCrystalI2C is not a drop-in replacement for old LiquidCrystal_I2C libraries.
It is a modern alternative designed for developers who care about:

    performance

    code quality

    long-term maintainability


# Sources / References


- Hitachi HD44780U Datasheet  
    https://www.sparkfun.com/datasheets/LCD/HD44780.pdf

- PCF8574 I²C I/O Expander Datasheet  
    https://www.nxp.com/docs/en/data-sheet/PCF8574.pdf

- HD44780 Instruction Set Summary  
    https://en.wikipedia.org/wiki/Hitachi_HD44780_LCD_controller#Instruction_set

- Huang, H. W. (2009). The hcs12/9s12: An introduction to software and hardware interfacing (2nd ed.), (pp. 323-338).Delmar Learning.
    ISBN 978-1-4354-2742-6.