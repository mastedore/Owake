/*
	time.cpp

	Implements custom time operations for the Owake project.


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

#include "time.hpp"
#include "rtc3231.h"

uint32_t getSecondsWithUnit(TimeUnit unit)
{
	switch (unit)
	{
	case second:
		return SECONDS;

	case minute:
		return MINUTES;

	case hour:
		return HOURS;

	default:
		return 0;
	}
}

uint32_t operateTime_24hrs(uint32_t t, bool forward, TimeUnit _unit)
{
	uint32_t unit = getSecondsWithUnit(_unit);

	if (forward)
	{
		t = (t + unit) % DAYS;
	}
	else
	{
		// Add a full day first so the unsigned value can't wrap below 00:00:00.
		t = (t + DAYS - unit) % DAYS;
	}

	return t;
}

uint8_t daysInMonth(const PackedDate &date)
{
    if (date.month == 2 && leap(EPOCH + date.year))
        return 29;

    return pgm_read_byte(&maxMonthDay[date.month - 1]);
}

void operateDate(PackedDate &date, bool forward, TimeUnit unit)
{
    if (date.month < 1 || date.month > 12)
        date.month = 1;

    uint8_t max = daysInMonth(date);

    switch (unit)
    {
    case day:

        if (forward)
        {
            // Compare before incrementing: day is a 5-bit field, so 31 + 1
            // would wrap to 0 before the "> max" check ever saw it.
            if (date.day >= max)
            {
                date.day = 1;
                operateDate(date, true, month);
            }
            else
            {
                date.day++;
            }
        }
        else
        {
            if (date.day > 1)
                date.day--;
            else
                date.day = max;
        }

        break;

    case month:

        if (forward)
        {
            date.month++;

            if (date.month > 12)
            {
                date.month = 1;
                operateDate(date, true, year);
            }
        }
        else
        {
            if (date.month > 1)
                date.month--;
            else
                date.month = 12;
        }

        break;

    case year:

        if (forward && date.year < MAX_PACKED_YEAR)
            date.year++;
        else if (!forward && date.year > 0)
            date.year--;

        break;

    default:
        return;
    }

    // Moving the month or year can leave the day past the end of the new
    // month (31/01 -> 31/02, or 29/02 into a common year), so pull it back.
    max = daysInMonth(date);
    if (date.day > max)
        date.day = max;
    else if (date.day < 1)
        date.day = 1;
}

// Sakamoto's method. Returns 1 = Monday ... 7 = Sunday, the range the DS3231 expects.
uint8_t dayOfWeek(const PackedDate &date)
{
    static const uint8_t offsets[12] PROGMEM = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};

    uint16_t y = static_cast<uint16_t>(EPOCH + date.year);
    if (date.month < 3)
        y--;

    uint8_t sunday0 = static_cast<uint8_t>(
        (y + y / 4 - y / 100 + y / 400 + pgm_read_byte(&offsets[date.month - 1]) + date.day) % 7);

    return static_cast<uint8_t>(((sunday0 + 6) % 7) + 1);
}