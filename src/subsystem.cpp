/*
    subsystem.cpp

    Implements TitaniumFSM (OwakeFSM) Subsystem class functions


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

#include "fault.h"

#include "subsystem.hpp"
#include "state.hpp"
#include "thislcd.hpp"

#define STATE_NOW (states + uint8_t(current))


void Subsystem::shiftMode(OwakeStateID mode)
{
	next = mode;
}

void Subsystem::requestShiftSubsystem(OwakeSubsystemID sys)
{
	//this->current = OwakeStateID::NONE;
	this->fsm->shiftSubsystem(sys);
}

void Subsystem::dispatch()
{
	if ((next == OwakeStateID::INVALID) || (current == OwakeStateID::INVALID))
	{
		FAULT(INVALID_STATE);
	}

	StateCtx ctx = {STATE_NOW};

	if ((next != current) && (next != OwakeStateID::NONE))
	{
		if ((current != OwakeStateID::NONE) && (current != OwakeStateID::INVALID))
		{
			STATE_NOW->main->exit(&ctx);
		}
		current = next;
		next = OwakeStateID::NONE;

		if (current != OwakeStateID::CLEANUP)
		{
			ctx.self = STATE_NOW;
			STATE_NOW->main->setup(&ctx);
		}
	}

	if ((current != OwakeStateID::NONE) && (current != OwakeStateID::CLEANUP))
	{
		STATE_NOW->main->loop(&ctx);
	}
}

void Subsystem::sectionMod(int8_t delta)
{
	int16_t tmp = (int16_t(section) + delta) % total_sections;
    if (tmp < 0) tmp += total_sections;
    section = uint8_t(tmp);
}

Subsystem::Subsystem(
	State* _states,
	uint8_t _sss,
	uint8_t _sections,
	uint32_t* _data,
	SubsystemRoutine* _routine):	states(_states),
									data(_data),
									routine(_routine),
									fsm(nullptr),
									total_sections(_sections),
									section(0)
{
	for (uint8_t i = 0; i < _sss; i++)
	{
		states[i].sys = this;
	}
}


