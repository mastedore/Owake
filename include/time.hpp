/*
    time.hpp

    Custom time operations for the Owake project.


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

#pragma once
#include <Arduino.h>

constexpr uint8_t SECONDS = 1u;
constexpr uint8_t MINUTES = 60u;
constexpr uint16_t HOURS = 3600u;
constexpr uint32_t DAYS = 86400ul;
constexpr uint32_t MAX_HOURS = 86399ul;
constexpr uint16_t EPOCH = 2026u; // epoch is 01/01/2026

enum TimeUnit : uint8_t
{
    hour = 0, minute = 1, second = 2, day = 3, month = 4, year = 5
};

struct PackedDate
{
    uint16_t year : 7;
    uint16_t month: 4;
    uint16_t day : 5;

    PackedDate(): year(0), month(1), day(1){}
    PackedDate(uint8_t _day, uint8_t _month, uint8_t _year): year(_year & 127), month(_month & 15), day(_day & 31){}
};

struct PackedTime
{
    uint16_t second : 6;
    uint16_t minute : 6;
    uint16_t hour : 5;

    PackedTime(): second(0), minute(0), hour(0){}
    PackedTime(uint8_t _s, uint8_t _m, uint8_t _h): second(_s & 63), minute(_m & 63), hour(_h & 31){}
    PackedTime(uint32_t tmstmp)
    {
	    const uint16_t rsec = static_cast<uint16_t>(tmstmp % HOURS);
        hour = tmstmp / HOURS;
        minute = rsec / 60u;
        second = rsec % 60u;
    }
};

uint32_t getSecondsWithUnit(TimeUnit unit);
uint32_t operateTime_24hrs(uint32_t t, bool forward, TimeUnit unit);
void operateDate(PackedDate &date, bool forward, TimeUnit unit);

inline bool leap(uint16_t num)
{
    return (num % 4) == 0;
}