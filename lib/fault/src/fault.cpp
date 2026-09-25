/*
    fault.cpp

    Implements YourFault.
    Better use with SolidCrystalI2C


    Copyright (c) 2025-2026 Mastedore <marcos@mastedore.com>

	This program is licensed under MIT license. See LICENSE file.
*/

#include <Arduino.h>
#include "fault.h"

// Checking SOLIDCRYSTAL_I2C here would never work, because that macro comes
// from the very header being included. Ask the compiler if the header exists.
#if __has_include("SolidCrystalI2C.h")
    #include "SolidCrystalI2C.h"
#endif


[[noreturn]] void FAULT(int8_t stopcode)
{
    //tone(11, static_cast<unsigned>(440 + stopcode * 94), 500);
#ifdef SOLIDCRYSTAL_I2C
    if (isThereAnyLCD())
    {
        //lcd << writeLayer::overlay << writeMode::text;
        lcd.clearAll();
        lcd.clear();
        lcd.home();
        lcd << "YourFault err D:";
        lcd.home(1);
        switch (stopcode)
        {
        case INVALID_STATE:
            lcd << "InvalidState";
            break;

        case PROGRAM_DONE:
            lcd << "Program Done :D";
            break;

        case SYMBOL_NOT_DEFINED:
            lcd << "SymbolNotDefined";
            break;

        case ILLEGAL_OPERATION:
            lcd << "IllegalOperation";
            break;

        case INVALID_SECTION:
            lcd << "InvalidSection";
            break;

        case DEVICE_TIMEOUT:
            lcd << "DeviceTimedOut";
            break;

        default:
            lcd << int32_t(stopcode);
            break;
        }
        lcd.flush();
    }
#endif
    // On AVR, 7 is WDTO_2S. The message stays up for about 2 s and then the
    // watchdog resets the board. HALT() only parks the CPU until that happens.
    WATCHDOG(7);
    HALT();
}