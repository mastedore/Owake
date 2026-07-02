/*
    fsm.hpp

    TitaniumFSM Community module for the Owake project.
	Implements OwakeFSM class which contains Subsystems,
	a safe subsystem shifter and a single dispatcher.


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
#include "Beverly.h"
#include "state.hpp"
#include "subsystem.hpp"
#include "types.hpp"

#define OWAKE_SUBSYSTEMS 5

class Subsystem;

class OwakeFSM
{
private:
	Subsystem* systems;
	OwakeSubsystemID current = OwakeSubsystemID::NONE;
	OwakeSubsystemID next = OwakeSubsystemID::MENU;

public:
	uint32_t timestamp = 0;
	void go();
	void shiftSubsystem(OwakeSubsystemID sys);
	OwakeFSM(Subsystem*, uint8_t);
};