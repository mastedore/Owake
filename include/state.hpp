/*
    state.hpp

    TitaniumFSM's State class module for Owake Subsystems and OwakeFSM.
	Implements State and Program classes
	Program is a struct that should have three function pointers:
		void(*setup),void(*loop),void(*exit)
	and is used by the State class as its behavior.
	State class has a pointer to a Subsystem object, corresponding to the subsystem where
	the State is located.

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
#include "SolidCrystalI2C.h"
#include "Beverly.h"
#include "fault.h"
#include "types.hpp"
#include "thislcd.hpp"
#include "time.hpp"
#include "main.hpp"

#include "subsystem.hpp"

#define STATE_FUNCTIONS \
    void setup(StateCtx* ctx); \
    void loop(StateCtx* ctx);  \
    void exit(StateCtx* ctx)




class State;
class Subsystem;

struct StateCtx
{
	State* self;
};


struct Program
{
	void (*setup)(StateCtx* ctx);
	void (*loop)(StateCtx* ctx);
	void (*exit)(StateCtx* ctx);
};

class State
{
public:
	const Program* main;
	Subsystem* sys = nullptr;
	State(const Program* prog): main(prog){};
};



namespace Menu {
	namespace Main{
		STATE_FUNCTIONS;
	}
}

namespace Clock {
	namespace Set{
		STATE_FUNCTIONS;
	}
	namespace BigView{
		STATE_FUNCTIONS;
	}
	namespace View{
		STATE_FUNCTIONS;
	}
}


namespace Alarm {
	extern uint8_t section_unit[ALARM_SECTIONS];
	extern bool confirmCancel;
	namespace Set{
		STATE_FUNCTIONS;
	}
	namespace Running{
		STATE_FUNCTIONS;
	}
	namespace Ringing{
		STATE_FUNCTIONS;
	}
}


namespace Chronometer {
	namespace Stop{
		STATE_FUNCTIONS;
	}
	namespace Running{
		STATE_FUNCTIONS;
	}
	namespace List{
		STATE_FUNCTIONS;
	}
}

namespace Credits{
	namespace Main{
		STATE_FUNCTIONS;
	}
}