# Facebits

A tiny bitmask manipulation library for embedded systems where every bit matters.

Designed to be lightweight, fast, and easy to use on microcontrollers such as AVR, ARM, and similar platforms.

## Features

- Simple bitmask read/write helpers
- Template-based (works with uint8_t, uint16_t, uint32_t, etc.)
- Zero dynamic memory usage
- Suitable for low-RAM environments

## Installation

### Arduino IDE
- Download the library as a ZIP
- Open Arduino IDE
- Sketch → Include Library → Add .ZIP Library

### Arduino Library Manager
Search for **Facebits** and install it directly.

## Usage

```cpp
#include <facebits.h>

uint8_t info = 0;
constexpr uint8_t MODE_MASK = 0b00000111;

// Write value into masked bits
writeData(info, MODE_MASK, 4);

// Read value from masked bits
uint8_t mode = readData(info, MODE_MASK); // 4
```
## API
```cpp
    T maskShift(mask)

    T maskWidth(mask)

    T readData(info, mask)

    void writeData(info, mask, data)

    T writeDataPreview(info, mask, data)
```


#### Licensed under BSD-2-clause
