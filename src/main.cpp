/*
	Owake - An open-source, alarm clock
	for nerds, hackers and makers.
	An Arduino thing i made to wake myself up in the
	mornings efficiently while still havin my phone close to me

	See README.md for details.

	main.cpp

	initializes hardware, create state, subsystem, fsm instances,
	then jump to fsm main routine fsm.go() in a loop



	Copyright (C) 2025-2026 Marcos Rubiano
	email:	markusianito@proton.me

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.

*/

#define OWK_FUNCTIONS(ns) \
	ns::setup,            \
		ns::loop,         \
		ns::exit

#include <Arduino.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>

#include "SolidCrystalI2C.h"
#include "Beverly.h"
#include "rtc3231.h"
#include "thislcd.hpp"
#include "state.hpp"
#include "subsystem.hpp"
#include "fsm.hpp"
#include "main.hpp"

volatile unsigned long ticks = 0;
volatile bool timerInt = false;
volatile bool buttonInt = false;

ISR(TIMER1_COMPA_vect)
{
	if (timerInt)
	{
		// didnt ++ because of Wvolatile
		ticks = ticks + 1;
		if (buttonInt)
		{
			BAction act = buttonOk.watch();
			if (act == BAction::Pressed || act == BAction::Released)
			{
				timerInt = false;
				return;
			}
		}
	}
}

void beep(uint16_t freq, uint16_t dura)
{
#if (BUZZER_TYPE)
	digitalWrite(BUZZER, true);
	delay(dura);
	digitalWrite(BUZZER, false);
#else
	tone(BUZZER, freq, dura);
#endif
}

const Program programs[] =
	{
		{OWK_FUNCTIONS(Menu::Main)},

		{OWK_FUNCTIONS(Clock::Set)},
		{OWK_FUNCTIONS(Clock::BigView)},
		{OWK_FUNCTIONS(Clock::View)},

		{OWK_FUNCTIONS(Alarm::Set)},
		{OWK_FUNCTIONS(Alarm::Running)},
		{OWK_FUNCTIONS(Alarm::Ringing)},

		{OWK_FUNCTIONS(Chronometer::Stop)},
		{OWK_FUNCTIONS(Chronometer::Running)},
		{OWK_FUNCTIONS(Chronometer::List)},

		{OWK_FUNCTIONS(Credits::Main)}};

State menuState(&programs[0]);
State clockStates[] = {
	State(&programs[1]),
	State(&programs[2]),
	State(&programs[3])};
State alarmStates[] = {
	State(&programs[4]),
	State(&programs[5]),
	State(&programs[6])};
State chronoStates[] = {
	State(&programs[7]),
	State(&programs[8]),
	State(&programs[9])};

State infoState(&programs[10]);

uint32_t alarmTime = 0;
uint32_t clockInfo = 0; // stores time and timestamp
uint32_t chronoNow = 0;

SubsystemRoutine menu_routine{&Routines::Menu::begin, &Routines::Menu::exit};
SubsystemRoutine chrono_routine{&Routines::Chronometer::begin, nullptr};

Subsystem systems[OWAKE_SUBSYSTEMS] = {
	Subsystem(&menuState, MENU_SUBSYSTEM_STATES, MENU_SECTIONS, nullptr, &menu_routine),
	Subsystem(clockStates, CLOCK_SUBSYSTEM_STATES, CLOCK_SECTIONS, &clockInfo, nullptr),
	Subsystem(alarmStates, ALARM_SUBSYSTEM_STATES, ALARM_SECTIONS, &alarmTime, nullptr),
	Subsystem(chronoStates, CHRONOMETER_SUBSYSTEM_STATES, CHRONOMETER_SECTIONS, &chronoNow, &chrono_routine),
	Subsystem(&infoState, CREDITS_SUBSYSTEM_STATES, INFO_SECTIONS, nullptr, nullptr)};

OwakeFSM fsm(systems, OWAKE_SUBSYSTEMS);

LCD lcd(LCD_ADDRESS, COLS, ROWS, true, false);

Button buttonOk(BUTTON_OK);
Button buttonDown(BUTTON_DOWN);
Button buttonUp(BUTTON_UP);

DS3231 rtc;

void setup()
{
	uint32_t ms;

	{	// Hardware setup routine
		MCUSR &= ~(1 << WDRF);
		wdt_disable();
		cli();

		TCCR1A = 0;
		TCCR1B = 0;
		TCCR1B |= (1 << WGM12); // CTC

		OCR1A = 249;
		TCCR1B |= (1 << CS11) | (1 << CS10);
		TIMSK1 |= (1 << OCIE1A); // compare A

		sei();
	}


	lcd.start();
	lcd.backlight(true);
	lcd.blink(true);
	lcd.cursor(false);
	defineHomeSymbols();
	lcd << writeMode::text << "Owake 1.0.0";
	lcd.home(1);
	lcd << "Getting Ready...";
	lcd.flush();
	delay(1000);
	{
		rtc.start();
		pinMode(BUZZER, OUTPUT);
		pinMode(ALARM_SIGNAL, OUTPUT);
		digitalWrite(BUZZER, LOW);
		digitalWrite(ALARM_SIGNAL, LOW);
		buttonOk.start();
		buttonDown.start();
		buttonUp.start();
	}
	lcd.home();
	ms = millis() - 1000UL;
	lcd << "Took " << ms << "ms" << cchar::endl;
	lcd.home(1);
	lcd << "Normally ~218ms";
	lcd.flush();
	delay(1000);
}

void loop()
{
	fsm.go();
}
