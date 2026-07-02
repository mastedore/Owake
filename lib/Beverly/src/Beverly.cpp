/*
    beverly.cpp

    Implements Button Events for AVR Easily.


    Copyright (C) 2025-2026 Marcos Rubiano
	email:	markusianito@proton.me

    SPDX-License-Identifier: MIT
*/

#include "Beverly.h"

ButtonFlags::ButtonFlags():
    PullState(0),
    ButtonHeld(0),
    ButtonReady(0),
    LastStable(0),
    LastRaw(0),
    WasHeld(0),
    Unused(0)
{

}

Button::Button(uint8_t _pin) : pin(_pin)
{

}

bool Button::wasHeld()
{
    return info.WasHeld;
}
void Button::discardHold()
{
    info.WasHeld = false;
}
bool Button::isReady()
{
    return info.ButtonReady;
}

uint8_t Button::getPin()
{
    return pin;
}
void Button::start(uint8_t pull_state)
{
    pinMode(pin, pull_state);
    info.ButtonReady = true;
    if (pull_state == INPUT_PULLUP)
    {
        info.PullState = true;
        info.LastRaw = true;
        info.LastStable = true;
    } else
    {
        info.PullState = false;
    }
}

BAction Button::watch()
{
    if (!info.ButtonReady) {return BAction::NotAvailable;}
    
    uint16_t now = static_cast<uint16_t>(millis());
    bool raw = digitalRead(pin);
    bool pullState = info.PullState;

    if (raw != info.LastRaw)
    {
        lastDebounceTime = now;
        info.LastRaw = raw;
    }

    if (uint16_t(now - lastDebounceTime) > DEBOUNCE_MS)
    {

        if (raw != info.LastStable)
        {
            info.LastStable = raw;

            if (pullState?!raw:raw) // if pullup, invert raw.
            {   // PRESS
                pressTime = now;
                info.WasHeld = false;
                return BAction::Pressed;
            }
            else
            {   // RELEASE
                info.ButtonHeld = false;
                return BAction::Released;
            }
        }
    }

    // LONG PRESS
    if (info.LastStable)
    {
        if (uint16_t(now - pressTime) >= LONGPRESS_MS)
        {
            info.ButtonHeld = true;
            info.WasHeld = true;
            return BAction::Held;
        }
    }
    return BAction::Idle;
}