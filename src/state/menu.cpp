/*
    menu.cpp

    Implements Owake menu subsystem state.


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

#include "state.hpp"
#include "subsystem.hpp"


void Routines::Menu::begin(Subsystem* sys)
{
	defineHomeSymbols();
	lcd.clear();
	lcd.home();
	lcd << "  " << Glyph::homie << " Owake Home" << cchar::endl;
	toggleArrows(true);
	lcd.home(1);
	// dont flush yet.
}

void Routines::Menu::exit(Subsystem* sys)
{
	lcd.clearAll();
	lcd.clear();
	lcd.home();
}


void Menu::Main::setup(StateCtx* ctx)
{
}

void Menu::Main::loop(StateCtx* ctx)
{
	Subsystem*& sys = ctx->self->sys;
	while (true)
	{
        wdt_reset();
        BAction act_ok = buttonOk.watch();
        BAction act_down = buttonDown.watch();
        BAction act_up = buttonUp.watch();

		if (act_down == BAction::Pressed)
		{
			sys->sectionMod(-1);
		}
		else if (act_up == BAction::Pressed)
		{
			sys->sectionMod(1);
		}
		else if (act_ok == BAction::Pressed)
		{
			sys->fsm->shiftSubsystem(static_cast<OwakeSubsystemID>(static_cast<uint8_t>(sys->section + 1U)));
			return;
		}
		
			

		lcd.home(1);
		switch (sys->section)
		{
		case 0:
			lcd << "    " << Glyph::clock << " Clock" << cchar::endl;
			break;

		case 1:
			lcd << "    " << Glyph::hourglass << " Alarm" << cchar::endl;
			break;

		case 2:
			lcd << ' ' << Glyph::chrono << " Chronometer" << cchar::endl;
			break;

		case 3:
			lcd << "    " << Glyph::man << " About" << cchar::endl;
			break;

		default:
			FAULT(INVALID_SECTION);
		}
		lcd.flush();
	}
}

void Menu::Main::exit(StateCtx* ctx)
{
	toggleArrows(false);
}