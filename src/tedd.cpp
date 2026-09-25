/*
    tedd.cpp
    Implements Time N Date Editor system for the Owake project

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

#include "Beverly.h"
#include "tedd.hpp"
#include "main.hpp"
#include "thislcd.hpp"
#include "fault.h"

#define dateShift 3
#define addSection(x) section = (section + x) % SECTIONS

#define writeTime()        \
    lcd.seti(ctx.textPos); \
    insertTime(timebuf, true, false, 0);

#define writeDate()        \
    lcd.seti(ctx.textPos); \
    insertDate(date, false, 0);

constexpr uint8_t SECTIONS = 5;
constexpr uint8_t NO_SECTION = 255;

// Sections are laid out as: 0 = cancel (X), 1-3 = the three fields, 4 = done.
// This maps each section to the TimeUnit it edits (hour, minute, second), with
// 255 for the two buttons. The date editor adds dateShift to get day, month, year.
static uint8_t sectionUnit[SECTIONS] = {255U, 0U, 1U, 2U, 255U};

static uint8_t section = 0;
// Section the cursor was last moved to. Moving it costs an I2C write, so
// adjustCursor() skips it unless the section actually changed.
static uint8_t counter = 0;

static inline void adjustTime(uint32_t &time, bool forward)
{
    uint8_t unit = sectionUnit[section];
    if (unit < 3)
    {
        time = operateTime_24hrs(time, forward, static_cast<TimeUnit>(unit));
    }
}

static inline void adjustDate(PackedDate &date, bool forward)
{
    uint8_t unit = sectionUnit[section];
    if (unit < 3)
    {
        operateDate(date, forward, static_cast<TimeUnit>(unit + dateShift));
    }
}

static inline void adjustCursor(EditorContext &ctx)
{
    if (counter == section)
        return;
    
    int pos;

    switch (section)
    {
    case 0:
        pos = ctx.cancelButtonPos;
        break;
    case 1:
        pos = ctx.textPos;
        break;
    // Each field is two digits plus a separator, so fields start 3 cells
    // apart in both "hh:mm:ss" and "dd/mm/yyyy".
    case 2:
        pos = ctx.textPos + 3;
        break;
    case 3:
        pos = ctx.textPos + 6;
        break;
    case 4:
        pos = ctx.doneButtonPos;
        break;
    default:
        return;
    }

    lcd.cursorPosition(static_cast<uint8_t>(pos));
    counter = section;
}

bool runTimeEditor(TimeEditorContext &ctx)
{
    section = 0;
    counter = NO_SECTION;
    lcd.seti(ctx.cancelButtonPos);
    lcd << 'X';
    lcd.seti(ctx.doneButtonPos);
    lcd << static_cast<char>(ctx.doneButtonChar);
    lcd.cursor(true);
    lcd.blink(true);

    // Work on a copy so cancelling leaves the caller's time untouched.
    uint32_t timebuf = *(ctx.time);
    writeTime();

    // Controls: on X or done, up/down move between sections. On a field,
    // up/down change the value and OK moves to the next field.

    bool done = false;
    while (!done)
    {
        wdt_reset();
        BAction act_ok = buttonOk.watch();
        BAction act_down = buttonDown.watch();
        BAction act_up = buttonUp.watch();
        if (act_ok == BAction::Pressed)
        {
            switch (section)
            {
            case 0:
                return done;

            case 4:
                *(ctx.time) = timebuf;
                done = true;
                break;

            case 1:
            case 2:
            case 3:
                addSection(1);
                break;

            default:
                FAULT(INVALID_SECTION);
            }
        }

        else if (act_down == BAction::Pressed || act_down == BAction::Held)
        {
            if (section == 0)
            {
                section = static_cast<uint8_t>(SECTIONS - 1U);
            }
            else if (section == static_cast<uint8_t>(SECTIONS - 1U))
            {
                --section;
            }
            else
            {
                adjustTime(timebuf, false);
                writeTime();
                lcd.flush();
            }
        }

        else if (act_up == BAction::Pressed || act_up == BAction::Held)
        {
            if (section == 0 || section == 4)
            {
                addSection(1);
            }
            else
            {
                adjustTime(timebuf, true);
                writeTime();
                lcd.flush();
            }
        }
        adjustCursor(ctx);
        lcd.flush();
    }

    return done;
}

bool runDateEditor(DateEditorContext &ctx)
{
    section = 0;
    counter = NO_SECTION;
    lcd.seti(ctx.cancelButtonPos);
    lcd << 'X';
    lcd.seti(ctx.doneButtonPos);
    lcd << static_cast<char>(ctx.doneButtonChar);
    lcd.cursor(true);
    lcd.blink(true);

    // Same controls as runTimeEditor().
    PackedDate date = *ctx.date;
    writeDate();

    bool done = false;
    while (!done)
    {
        wdt_reset();
        BAction act_ok = buttonOk.watch();
        BAction act_down = buttonDown.watch();
        BAction act_up = buttonUp.watch();
        if (act_ok == BAction::Pressed)
        {
            switch (section)
            {
            case 0:
                return done;

            case 4:
                *(ctx.date) = date;
                done = true;
                break;

            case 1:
            case 2:
            case 3:
                addSection(1);
                break;

            default:
                FAULT(INVALID_SECTION);
            }
        }

        else if (act_down == BAction::Pressed || act_down == BAction::Held)
        {
            if (section == 0)
            {
                section = static_cast<uint8_t>(SECTIONS - 1U);
            }
            else if (section == static_cast<uint8_t>(SECTIONS - 1U))
            {
                --section;
            }
            else
            {
                adjustDate(date, false);
                writeDate();
                lcd.flush();
            }
        }
        else if (act_up == BAction::Pressed || act_up == BAction::Held)
        {
            if (section == 0 || section == 4)
            {
                addSection(1);
            }
            else
            {
                adjustDate(date, true);
                writeDate();
                lcd.flush();
            }
        }
        adjustCursor(ctx);
        lcd.flush();
    }

    return done;
}