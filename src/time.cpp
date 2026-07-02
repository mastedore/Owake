/*
	time.cpp

	Implements custom time operations for the Owake project.


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
		if ((t+=unit) > MAX_HOURS)
			t = 0;
	}
	else
	{
		if ((t-=unit) > MAX_HOURS)
			t = 0;
	}
	
	return t;
}
void operateDate(PackedDate &date, bool forward, TimeUnit unit)
{
    bool isleep = leap(EPOCH + date.year);

    if (date.month < 1 || date.month > 12)
        date.month = 1;

    uint8_t max =
        (date.month == 2 && isleep) ?
        29 : maxMonthDay[date.month - 1];

    switch (unit)
    {
    case day:

        if (forward)
        {
            date.day++;

            if (date.day > max)
            {
                date.day = 1;
                operateDate(date, true, month);
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

        if (forward)
            date.year++;
        else if (date.year > 0)
            date.year--;

        break;
    }
}