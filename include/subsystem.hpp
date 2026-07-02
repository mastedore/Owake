/*
    subsystem.hpp

    TitaniumFSM Subsystem module for the OwakeFSM.
	a Subsystem is an object containing State objects,
	intended for separate modes / apps in the FSM.
	Subsystem has a single dispatcher which calls a State's Program functions
	depending of the context (it decides between exit, loop and setup).
	Subsystem also implements SubsystemRoutine, a struct containing 2 function pointers,
	these functions are called once when the subsystem starts, or exits (is shifted in the OwakeFSM)


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
#define MENU_SECTIONS 4
#define ALARM_SECTIONS 5
#define CHRONOMETER_SECTIONS 1
#define INFO_SECTIONS 1
#define CLOCK_SECTIONS 7

#define CLOCK_SECTION_MODE	0b00000001
#define CLOCK_SECTION_RTC 	0b00000010
#define CLOCK_SECTION_FONT 	0b00000100

#define MENU_SUBSYSTEM_STATES 1
#define CLOCK_SUBSYSTEM_STATES 3
#define ALARM_SUBSYSTEM_STATES 3
#define CHRONOMETER_SUBSYSTEM_STATES 3
#define CREDITS_SUBSYSTEM_STATES 1


#define CHRONOMETER_MAX_LIST 10

#include <Arduino.h>
#include "Beverly.h"

#include "fsm.hpp"
#include "state.hpp"
#include "types.hpp"



class State;
class OwakeFSM;
class Subsystem;

struct SubsystemRoutine
{
	void (*begin)(Subsystem*);
	void (*cleanup)(Subsystem*);
};

class Subsystem
{
	public:
	State* states;
    uint32_t* data; //void* context;
	SubsystemRoutine* routine;
	OwakeFSM* fsm = nullptr;
	uint8_t total_sections;
	uint8_t section;
	OwakeStateID current = OwakeStateID::NONE;
	OwakeStateID next = OwakeStateID::NONE;
	void shiftMode(OwakeStateID mode);
	void dispatch();
	void requestShiftSubsystem(OwakeSubsystemID sys);
	void sectionMod(int8_t delta);
	Subsystem(State*, uint8_t, uint8_t, uint32_t*, SubsystemRoutine*);
};



namespace Routines
{
	namespace Menu
	{
		void begin(Subsystem*);
		void exit(Subsystem*);
	}

	namespace Chronometer
	{
		void begin(Subsystem*);
		//void exit(Subsystem*);
	}
}