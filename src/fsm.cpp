/*
    fsm.cpp

	Implements TitaniumFSM class functions.


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

#include <avr/wdt.h>

#include "fsm.hpp"
#include "fault.h"

#define SYS_NOW (systems + uint8_t(current))
#define SYS_STATE_NOW SYS_NOW->states[(uint8_t(SYS_NOW->current))]

void OwakeFSM::shiftSubsystem(OwakeSubsystemID sys)
{
	next = sys;
}

void OwakeFSM::go()
{
	wdt_reset();

	if (next == OwakeSubsystemID::INVALID)
	{
		FAULT(INVALID_STATE);
	}
	if ((next != current) && (next != OwakeSubsystemID::NONE))
	{
		if ((current != OwakeSubsystemID::NONE) && (current != OwakeSubsystemID::INVALID))
		{
			// 2 vulnerable instructions below
			StateCtx ctx{&SYS_STATE_NOW};
			SYS_STATE_NOW.main->exit(&ctx);
			if ((SYS_NOW->routine != nullptr) && (SYS_NOW->routine->cleanup != nullptr))
			{
				SYS_NOW->routine->cleanup(SYS_NOW);
			}
			SYS_NOW->current = OwakeStateID::NONE;
		}
		current = next;
		next = OwakeSubsystemID::NONE;

		if ((SYS_NOW->routine != nullptr) && (SYS_NOW->routine->begin != nullptr))
		{
			SYS_NOW->routine->begin(SYS_NOW);
		}
		SYS_NOW->shiftMode(OwakeStateID::INIT);
	}

	if (current != OwakeSubsystemID::NONE)
	{
		SYS_NOW->dispatch();
	}
}

OwakeFSM::OwakeFSM(Subsystem *_syses, uint8_t total) : systems(_syses)
{
	for (uint8_t i = 0; i < total; i++)
	{
		_syses[i].fsm = this;
	}
}