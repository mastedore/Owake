/*
    tedd.hpp
    User mode Time and Date Editor system for the Owake project

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

#include <stdint.h>
#include "time.hpp"

struct EditorCtxFlags
{
    uint8_t format : 1;
    uint8_t unused : 7;

    EditorCtxFlags(bool fmt=0): format(fmt){}
};

struct EditorContext
{
    uint8_t cancelButtonPos;
    uint8_t doneButtonPos;
    uint8_t textPos;
    uint8_t doneButtonChar;
    EditorCtxFlags flags;

    EditorContext(
        uint8_t cancelPos,
        uint8_t donePos,
        uint8_t _textPos,
        uint8_t doneChar,
        bool _format=0):
                        cancelButtonPos(cancelPos),
                        doneButtonPos(donePos),
                        textPos(_textPos),
                        doneButtonChar(doneChar)
    {}
};

struct TimeEditorContext : public EditorContext
{
    uint32_t* time;
    TimeEditorContext(uint32_t* t, uint8_t cancelPos, uint8_t donePos, uint8_t _textPos, uint8_t doneChar, bool fmt=0)
        :   EditorContext(cancelPos, donePos, _textPos, doneChar, fmt),
            time(t)
    {}
};

struct DateEditorContext : public EditorContext
{
    PackedDate* date;
    DateEditorContext(PackedDate* d, uint8_t cancelPos, uint8_t donePos, uint8_t _textPos, uint8_t doneChar, bool fmt=false)
        :   EditorContext(cancelPos, donePos, _textPos, doneChar, fmt),
            date(d)
    {}
};


bool runTimeEditor(TimeEditorContext&);
bool runDateEditor(DateEditorContext&);