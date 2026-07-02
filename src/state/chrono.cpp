/*
	chrono.cpp

	Implements chronometer subsystem states for Owake.


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
#include "subsystem.hpp"
#include "main.hpp"
#include "thislcd.hpp"

#define time *(sys->data)


enum Bmode : bool
{
	Hold = false,
	Press = true
};

bool mode = Bmode::Press;
bool running = false;
bool thicc = true;
uint8_t font = 1;



static void displaySettings(bool visible=true)
{
	uint8_t position = COLS-2;
	lcd.row(0);
	lcd.column(position);
	lcd << (running ? 'O' : 'X');
	lcd << font;
	
	lcd.row(1);
	lcd.column(position);
	lcd << (thicc ? 'M' : 'm');
	lcd << ((mode==Bmode::Press) ? 'P' : 'H');
}

static void displayMillis()
{
	lcd.seti(3);
	insertMillis(ticks, thicc, (thicc ? font : 0));
}



void Routines::Chronometer::begin(Subsystem*)
{
	ticks = 0;
	lcd.cursor(false);
    defineFont(1);
	lcd.home(1);
	lcd << '>';
	lcd.home();
	lcd << '>';
}


void Chronometer::Stop::setup(StateCtx* ctx)
{
	displayMillis();
	displaySettings();
	lcd.flush();
}

void Chronometer::Stop::loop(StateCtx* ctx)
{
	Subsystem*& sys = ctx->self->sys;
	BAction act_ok = buttonOk.watch();
	BAction act_down = buttonDown.watch();
	BAction act_up = buttonUp.watch();

	if (mode == Bmode::Press)
	{
		if (act_ok == BAction::Pressed)
		{
			sys->shiftMode(OwakeStateID::CHRONOMETER_RUNNING);
			return;
		}
	}
	else if (mode == Bmode::Hold)
	{
		// pending
	}


	if (act_up == BAction::Released)
	{
		if (buttonUp.wasHeld())
		{
			font = (font % DIGIT_FONTS) + 1;
			defineFont(font);
			return;
		}
		else	// was just pressed
		{
			mode = !mode;
		}
	}
	else if (act_down == BAction::Released)
	{
		if (buttonDown.wasHeld())
		{
			buttonUp.discardHold();
			sys->requestShiftSubsystem(OwakeSubsystemID::MENU);
			return;
		}
		else	// was just pressed
		{
			thicc = !thicc;
		}
	}
	
	displaySettings();
	lcd.flush();
}

void Chronometer::Stop::exit(StateCtx* ctx)
{

}

void Chronometer::Running::setup(StateCtx* ctx)
{
	running = true;
	buttonInt = true;
	timerInt = true;
}

void Chronometer::Running::loop(StateCtx* ctx)
{
	// timerInt sets itself to false when buttonOk is Pressed.
	if (!timerInt)
	{
		ctx->self->sys->shiftMode(OwakeStateID::CHRONOMETER_STOP);
		return;
	}
	
	displayMillis();
	displaySettings();
	lcd.flush();
}

void Chronometer::Running::exit(StateCtx* ctx)
{
	running = false;
}
















// todo soon
// deleted by -flto if empty
void Chronometer::List::setup(StateCtx* ctx)
{

}

void Chronometer::List::loop(StateCtx* ctx)
{

}

void Chronometer::List::exit(StateCtx* ctx)
{

}