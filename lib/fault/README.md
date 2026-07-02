# YourFault
Provides simple critical error handling for AVR and ESP microcontrollers.

Implements `HALT()` which stops program execution forever
and `FAULT(char stopcode)` which calls `HALT()` with `tone()` depending on the stopcode.
It can use default lcd created in [SolidCrystalI2C](https://github.com/markusianito/SolidCrystalI2C) to display stopcode.

Licensed under MIT.