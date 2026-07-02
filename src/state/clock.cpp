/*
	clock.cpp

	Implements clock subsystem states for Owake.


	Copyright (c) 2025-2026 Mastedore <marcos@mastedore.com>

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


#include "state.hpp"
#include "rtc3231.h"
#include "time.hpp"
#include "tedd.hpp"
#include "main.hpp"

#define ACTUAL_SECTIONS 2
#define time *(sys->data)

#define shiftSession() sys->shiftMode(static_cast<OwakeStateID>((sys->section % ACTUAL_SECTIONS) + 1))

static PackedDate date;
static bool rtcOn = false;
static bool timedef = false;
static uint32_t baseTime;
static uint32_t baseMillis;

static void restoreSession(Subsystem *&sys, bool _rtc)
{
	uint8_t newSection = sys->section % ACTUAL_SECTIONS;
	if (_rtc)
	{
		newSection += ACTUAL_SECTIONS;
	}
	sys->section = newSection;
}

static void refreshTime(Subsystem *&sys)
{
	if (sys->section == 0 || sys->section == 1)
	{
		uint32_t now = millis();
		uint32_t elapsed = (now - baseMillis) / 1000;

		time = baseTime + elapsed;

		while (time >= DAYS)
		{
			time -= DAYS;
			operateDate(date, true, day);
		}

		baseTime = time;
		baseMillis = now - (now - baseMillis) % 1000;
	}
	else if (sys->section & 0x2)
	{
		rtc.updateTime();
		uint32_t seconds = static_cast<uint32_t>(rtc.timestamp.getSecond());
		uint32_t minutes = static_cast<uint32_t>(rtc.timestamp.getMinute()) * MINUTES;
		uint32_t hours = static_cast<uint32_t>(rtc.timestamp.getHour()) * HOURS;
		time = seconds + minutes + hours;
	}
	else
	{
		FAULT(INVALID_SECTION);
	}
}

static void fgui()
{
	lcd.home();
	lcd << '*';
	lcd.home(1);
	lcd << static_cast<char>(127);
}

/*

	Ask ds3231 for time, if a valid time is found,
		then jump to Clock::BigView/View depending on previous setting (BigView default)
	otherwise, ask user to set the time and set it to ds3231.


	No ds3231? Fine, ask user to set the time then use ctx->self->data pointer to store timestamp
	Epoch for Owake is 01/01/2026 00:00:00 UTC.
	However, the first valid timestamp is 01/01/2026 00:04:15 UTC

	!timedef?
		update timestamp according to elapsed time on millis(),
		because other subsystems but Clock can run while the timestamp is still set
		then jump to Clock::BigView/View depending on previous setting (BigView default)
	otherwise ask user to set the time
*/
void Clock::Set::setup(StateCtx *ctx)
{
	lcd.clear();
	Subsystem *&sys = ctx->self->sys;

	if (rtc.isAvailable() == 0) // rtc available.
	{
		rtcOn = true;
		// osf flag on, aka setted time unreliable
		if (rtc.flags.osf)
		{
			return;
		} // go to clock.set.loop

		// all correct
		restoreSession(sys, true);
		shiftSession();
	}
	else
	{ // rtc NOT available.

		if (rtcOn)
		{
			rtcOn = false;
			return;
		}
		// time not defined
		if (!timedef)
		{
			return;
		} // this will make the subsystem dispatch clock.set.loop

		// time defined
		rtcOn = false;
		restoreSession(sys, false);
		shiftSession();
	}
}

void Clock::Set::loop(StateCtx *ctx)
{
	Subsystem *&sys = ctx->self->sys;

	TimeEditorContext tectx(sys->data, COLS, (COLS * 2) - 1, COLS + 4, 4);
	DateEditorContext dactx(&date, COLS, (COLS * 2) - 1, COLS + 3, 4, true);

	lcd.home();
	lcd << "What time is it?";

	if (!runTimeEditor(tectx))
	{
		goto jump2menu;
	}
	baseTime = time;
	baseMillis = millis();

	lcd.home();
	lcd << "& Today's date? ";
	if (!runDateEditor(dactx))
	{
		goto jump2menu;
	}

	if (rtcOn)
	{
		PackedTime tyme(time);

		TimestampDS3231 tstamp;
		tstamp.setHourMode(HMODE_24); // todo: customizable ampm
		tstamp.setCentury(false);

		tstamp.setDay(date.day);
		tstamp.setMonth(date.month);
		tstamp.setYear(date.year);
		tstamp.setHour(tyme.hour);
		tstamp.setMinute(tyme.minute);
		tstamp.setSecond(tyme.second);
	}

	timedef = true;
	sys->shiftMode(OwakeStateID::CLOCK_BIG_VIEW);
	return;

jump2menu:
	sys->requestShiftSubsystem(OwakeSubsystemID::MENU);
	return;
}

void Clock::Set::exit(StateCtx *ctx)
{
}

void Clock::BigView::setup(StateCtx *ctx)
{
	lcd.clearAll();
	lcd.clear();
	defineFont(1);
	lcd.cursor(false);
	lcd.blink(false);

	fgui();

	Subsystem *&sys = ctx->self->sys;
}

void Clock::BigView::loop(StateCtx *ctx)
{
	Subsystem *&sys = ctx->self->sys;

	refreshTime(sys);

	if (rtcOn)
	{
		lcd.seti(COLS - 2);
		lcd << (int16_t)rtc.itemperature();
		lcd.seti((2 * COLS) - 3);
		lcd << "'C";
	}

	lcd.seti(4);
	insertTime(time, true, true, 1);
	lcd.flush();

	BAction act_ok = buttonOk.watch();
	BAction act_down = buttonDown.watch();
	// BAction act_up = buttonUp.watch();

	if ((act_ok == BAction::Pressed) || (act_ok == BAction::Held))
	{
		sys->shiftMode(OwakeStateID::CLOCK_VIEW);
		return;
	}
	else if ((act_down == BAction::Pressed) || (act_down == BAction::Held))
	{
		sys->requestShiftSubsystem(OwakeSubsystemID::MENU);
		return;
	}
}

void Clock::BigView::exit(StateCtx *ctx)
{
	Subsystem *&sys = ctx->self->sys;
}

void Clock::View::setup(StateCtx *ctx)
{
	lcd.clearAll();
	lcd.clear();
	lcd.cursor(false);
	lcd.blink(false);

	fgui();
}

void Clock::View::loop(StateCtx *ctx)
{
	Subsystem *&sys = ctx->self->sys;

	refreshTime(sys);

	lcd.home();
	insertTime(time, false, false, 0);

	lcd.home(1);
	insertDate(date, false, 0);

	if (rtcOn)
	{
		int16_t temp = rtc.itemperature();
		int16_t mantis = rtc.temperatureMantis();
		lcd.seti(8);
		lcd << temp;
		lcd << '.';
		lcd << mantis;
		lcd << "'C";
	}
	lcd.flush();

	BAction act_ok = buttonOk.watch();
	BAction act_down = buttonDown.watch();
	// BAction act_up = buttonUp.watch();

	if ((act_ok == BAction::Pressed) || (act_ok == BAction::Held))
	{
		sys->shiftMode(OwakeStateID::CLOCK_BIG_VIEW);
		return;
	}
	else if ((act_down == BAction::Pressed) || (act_down == BAction::Held))
	{
		sys->requestShiftSubsystem(OwakeSubsystemID::MENU);
		return;
	}
}

void Clock::View::exit(StateCtx *ctx)
{
}