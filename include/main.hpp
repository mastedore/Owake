#pragma once

#define BUZZER 11
#define BUTTON_OK 4
#define BUTTON_UP 5
#define BUTTON_DOWN 6
#define ALARM_SIGNAL 7 // high when an alarm is ringing, otherwise low

#define ROWS 2
#define COLS 16
#define LCD_ADDRESS 0x27

#define BUZZER_TYPE true // true for active, false for passive

#include <stdint.h>
extern volatile bool buttonInt;
extern volatile bool timerInt;
extern volatile uint32_t ticks;


#include "SolidCrystalI2C.h"
#include "Beverly.h"
#include "rtc3231.h"


extern LCD lcd;
extern Button buttonOk;
extern Button buttonDown;
extern Button buttonUp;
extern DS3231 rtc;

void beep(uint16_t freq, uint16_t duration);
//int16_t fahrenheit(int16_t celsius);