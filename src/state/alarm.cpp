/*
    alarm.cpp

    Implements Owake alarm subsystem states, such as Set, Running & Ringing.


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
    along with thnis program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "state.hpp"
#include "main.hpp"
#include "tedd.hpp"

#define time *(sys->data)
#define LCD_BACKLIGHTOFF_S 5

uint8_t Alarm::section_unit[ALARM_SECTIONS] =
    {
        255,
        2,
        1,
        0,
        255};

bool Alarm::confirmCancel = false;


void Alarm::Set::setup(StateCtx *ctx)
{
    lcd.clearAll();
    lcd.clear();
    Subsystem *&sys = ctx->self->sys;
    tone(BUZZER, 1975, 30);
    lcd.home();
    lcd << ' ' << Glyph::hourglass << " Set an Alarm" << cchar::endl;
    lcd.seti(static_cast<uint8_t>(4 + COLS));
    insertTime(time, true, false, 0);
    lcd.cursorPosition(0, 1);   
    lcd.flush();
}

void Alarm::Set::loop(StateCtx *ctx)
{
    Subsystem *&sys = ctx->self->sys;
	TimeEditorContext tectx(sys->data, COLS, (COLS*2)-1, COLS+4, 3);

    bool done = runTimeEditor(tectx);

    if (done)
    {
        sys->shiftMode(OwakeStateID::ALARM_RUNNING);
    }
    else
    {
        sys->requestShiftSubsystem(OwakeSubsystemID::MENU);
        return;
    }
}

void Alarm::Set::exit(StateCtx *ctx)
{
    ctx->self->sys->section = 0;
    tone(BUZZER, 1890, 30);
}

void Alarm::Running::setup(StateCtx *ctx)
{
    lcd.home();
    lcd << Glyph::hourglass << " Waking up in..";
    lcd.home(1);
    lcd << Glyph::bar << Glyph::right;
    lcd.row(15);
    lcd << 'X';
    lcd.cursorPosition(15, 1);
    ctx->self->sys->fsm->timestamp = millis();
}

static uint8_t timer = LCD_BACKLIGHTOFF_S;
inline void handleTimer()
{
    if (timer == 0U) {lcd.backlight(false);}
    else {lcd.backlight(true);}
}
void Alarm::Running::loop(StateCtx *ctx)
{
    handleTimer();
    Subsystem *&sys = ctx->self->sys;
    uint32_t now = millis();
    if ((now - sys->fsm->timestamp) >= 1000)
    {
        uint32_t elapsed = (now - sys->fsm->timestamp) / 1000;
        time -= elapsed;
        sys->fsm->timestamp += elapsed * 1000;
        lcd.seti(static_cast<uint8_t>(4 + COLS));
        insertTime(time, true, false, 0);
        lcd.flush();
        if (timer > 0) --timer;
    }
    BAction act_ok = buttonOk.watch();
    BAction act_down = buttonDown.watch();
    BAction act_up = buttonUp.watch();
    if (act_ok == BAction::Held)
    {
        if (!confirmCancel)
        {
            confirmCancel = true;
            lcd.row(1);
            lcd.column(static_cast<uint8_t>(lcd.getColumns() - 7));
            lcd << writeLayer::overlay;
            lcd << "Cancel?";
            lcd << writeLayer::user;
            lcd.flush();
        }
    }
    else if (act_ok == BAction::Pressed)
    {
        if (confirmCancel)
        {
            tone(BUZZER, 1900, 60);
            sys->shiftMode(OwakeStateID::ALARM_SET);
            return;
        }
        else
        {
            timer = LCD_BACKLIGHTOFF_S;
            tone(BUZZER, 440, 40);
        }
    }
    else if ((act_down != BAction::Idle) || (act_up != BAction::Idle))
    {
        if (confirmCancel)
        {
            confirmCancel = false;
            lcd.row(1);
            lcd.column(static_cast<uint8_t>(lcd.getColumns() - 7));
            lcd << writeLayer::overlay;
            for (uint8_t i = 0U; i <= 7U; i++)
            {
                lcd << cchar::skip;
            }
            lcd << writeLayer::user;
            lcd.flush();
        }
        timer = LCD_BACKLIGHTOFF_S;
    }

    if (time == 0U)

    {
        sys->shiftMode(OwakeStateID::ALARM_RINGING);
    }
}

void Alarm::Running::exit(StateCtx *ctx)
{
    lcd.clear();
    lcd.constantCursor(false);
    ctx->self->sys->fsm->timestamp = 0;
    *(ctx->self->sys->data) = 0;
}

void Alarm::Ringing::setup(StateCtx *ctx)
{
    lcd.backlight(true);
    lcd << writeMode::text;
    lcd.home();
    lcd << Glyph::right << " WAKE UP! " << cchar::endl;
    lcd.home(1);
    lcd << Glyph::bar << Glyph::right;
    lcd.row(15);
    lcd << Glyph::man;

    digitalWrite(ALARM_SIGNAL, HIGH);
}

void Alarm::Ringing::loop(StateCtx *ctx)
{
    BAction act_ok = buttonOk.watch();
    if (act_ok == BAction::Held)
    {
        ctx->self->sys->requestShiftSubsystem(OwakeSubsystemID::MENU);
        return;
    }
    beep(440, 500);
    lcd.row(1);
    lcd.column(3);

    lcd.flush();
}

void Alarm::Ringing::exit(StateCtx *ctx)
{
    digitalWrite(BUZZER, LOW);
    digitalWrite(ALARM_SIGNAL, LOW);
    lcd << writeMode::text;
    lcd.home();
}