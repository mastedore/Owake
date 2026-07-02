/*
    fault.hpp

    Provides easy error handling for AVR or ESP microcontrollers



    Copyright (C) 2026 Marcos Rubiano
	email:	markusianito@proton.me

	This program is licensed under MIT license. See LICENSE file.
*/

#ifndef FAULT_HPP
#define FAULT_HPP

#define FAULT_BUZZER_PIN 11

#define INVALID_SECTION -2
#define INVALID_STATE -1
#define PROGRAM_DONE 0
#define SYMBOL_NOT_DEFINED 1
#define ILLEGAL_OPERATION 2
#define DEVICE_TIMEOUT 3

#if defined(__AVR__)
    #include <avr/wdt.h>
    #define HALT()                      \
        {                               \
            asm volatile("cli");        \
            while (true)                \
            {                           \
                asm volatile("sleep");  \
            }                           \
        }
    #define WATCHDOG(t) wdt_enable(t)

#elif defined(ESP8266) || defined(ESP32)
    #include "esp_task_wdt.h"
    #define WATCHDOG(t)                 \
    {                                   \
        esp_task_wdt_init(5, true);     \
        esp_task_wdt_add(NULL);         \
    }                                   

    #define HALT()                      \
    {                                   \
        portDISABLE_INTERRUPTS();       \
        while (true)                    \
        {                               \
            esp_deep_sleep_start();     \
        {                               \
    }                                   
#else
    #warning fault.hpp works better on AVR and ESP32/8266
    #define HALT() \
    {\
        while (true){\

        }\
    }
    #define WATCHDOG(t)\
    {\
        for (char i = t; i <= t; i++){\
        }\
    }
#endif

[[noreturn]] void FAULT(int8_t stopcode);



#endif // FAULT_HPP