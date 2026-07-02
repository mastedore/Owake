/*
    types.hpp

    Defines Owake's TitainiumFSM Subsystem and State identifiers.


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

#pragma once
#include <Arduino.h>

enum class OwakeStateID : uint8_t {
	MENU_MAIN = 0,
	
	CLOCK_SET = 0,
	CLOCK_BIG_VIEW = 1,
	CLOCK_VIEW = 2,
	
	
	ALARM_SET = 0,
	ALARM_RUNNING = 1,
	ALARM_RINGING = 2,
	
	
	CHRONOMETER_STOP = 0,
	CHRONOMETER_RUNNING = 1,
	CHRONOMETER_LIST = 2,

	CREDITS_MAIN = 0,
	
	INIT = 0,
	EXIT = 252,
	CLEANUP = 253,
	NONE = 254,
	INVALID = 255
};


enum class OwakeSubsystemID : uint8_t {
	MENU = 0,
	CLOCK = 1,
	ALARM = 2,
	CHRONOMETER = 3,
	INFO = 4,
	NONE = 254,
	INVALID = 255
};