/*
	Connected Discord-GitHub
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

// The clock's section works as two flags. Bit 0 picks the view (0 = big
// digits, 1 = small view with the date) and bit 1 means the time comes from
// the DS3231 instead of millis().
#define ACTUAL_SECTIONS 2
#define SECTION_SMALL_VIEW 0x1
#define SECTION_RTC 0x2
#define time *(sys->data)

// CLOCK_BIG_VIEW is 1 and CLOCK_VIEW is 2, hence the + 1.
#define shiftSession() sys->shiftMode(static_cast<OwakeStateID>((sys->section % ACTUAL_SECTIONS) + 1))

static PackedDate date;
static bool rtcOn = false;
static bool timedef = false;
// Without an RTC, the current time is baseTime plus however long millis()
// says it has been since baseMillis.
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

		// A while and not an if: the clock may have been away for more than one
		// midnight while another subsystem was running.
		while (time >= DAYS)
		{
			time -= DAYS;
			operateDate(date, true, day);
		}

		baseTime = time;
		// Keep the leftover milliseconds, or every refresh would lose a bit of time.
		baseMillis = now - (now - baseMillis) % 1000;
	}
	else if (sys->section & SECTION_RTC)
	{
		// Keep showing the last good reading if the bus hiccups.
		if (rtc.updateTime() != 0)
		{
			return;
		}

		uint8_t hour24 = rtc.timestamp.getHour();
		if (rtc.timestamp.getHourMode() == HMODE_12)
		{
			hour24 = static_cast<uint8_t>((hour24 % 12) + ((rtc.timestamp.getMD() == PM) ? 12 : 0));
		}

		uint32_t seconds = static_cast<uint32_t>(rtc.timestamp.getSecond());
		uint32_t minutes = static_cast<uint32_t>(rtc.timestamp.getMinute()) * MINUTES;
		uint32_t hours = static_cast<uint32_t>(hour24) * HOURS;
		time = seconds + minutes + hours;

		// The chip stores years since 2000, PackedDate counts from EPOCH.
		uint16_t fullYear = static_cast<uint16_t>(2000u + rtc.timestamp.getYear() + (rtc.timestamp.getCentury() ? 100u : 0u));
		uint16_t packedYear = (fullYear > EPOCH) ? static_cast<uint16_t>(fullYear - EPOCH) : 0u;
		date = PackedDate(
			rtc.timestamp.getDay(),
			rtc.timestamp.getMonth(),
			static_cast<uint8_t>((packedYear > MAX_PACKED_YEAR) ? MAX_PACKED_YEAR : packedYear));
	}
	else
	{
		FAULT(INVALID_SECTION);
	}
}

// Small markers in the left column shared by both views.
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
		// It may have been plugged in after boot, and setTime() refuses to
		// write until start() has run and read the OSF flag.
		if (!rtc.flags.started)
		{
			rtc.start();
		}
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

		// The RTC was the time source and it's gone now. millis() was never
		// synced to it, so ask for the time again.
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
		uint8_t yearsSince2000 = static_cast<uint8_t>(date.year + (EPOCH - 2000u));

		TimestampDS3231 tstamp;
		tstamp.setHourMode(HMODE_24); // todo: customizable ampm
		tstamp.setCentury(yearsSince2000 >= 100);

		// Year and month go before the day because setDay() clamps
		// against them (days in month, leap years).
		tstamp.setYear(yearsSince2000);
		tstamp.setMonth(date.month);
		tstamp.setDay(date.day);
		tstamp.setWeekDay(dayOfWeek(date));
		tstamp.setHour(tyme.hour);
		tstamp.setMinute(tyme.minute);
		tstamp.setSecond(tyme.second);

		// If the write fails, keep going on millis() (baseTime is already set).
		if (rtc.setTime(tstamp) != 0)
		{
			rtcOn = false;
		}
	}

	timedef = true;
	restoreSession(sys, rtcOn);
	shiftSession();
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
		sys->section |= SECTION_SMALL_VIEW;
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
		sys->section &= static_cast<uint8_t>(~SECTION_SMALL_VIEW);
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