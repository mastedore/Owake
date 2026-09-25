/*
    beverly.cpp

    Implements Button Events for AVR Easily.


    Copyright (c) 2025-2026 Mastedore <marcos@mastedore.com>

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
// With INPUT_PULLUP the pin idles HIGH and reads LOW when pressed, so the
// "last" states start HIGH to avoid a fake press on the first watch().
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
    
    // Only the low 16 bits of millis() are kept to save RAM. The uint16_t
    // subtractions below still give the right gap as long as it's under ~65 s.
    uint16_t now = static_cast<uint16_t>(millis());
    bool raw = digitalRead(pin);
    bool pullState = info.PullState;

    if (raw != info.LastRaw)
    {
        lastDebounceTime = now;
        info.LastRaw = raw;
    }

    // The pin has to hold the same level for DEBOUNCE_MS before it counts
    // as a real edge.
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

    // LONG PRESS. Held comes back on every call for as long as the button stays
    // down, not just once. Callers that want a single event can wait for
    // Released and check wasHeld().
    if (pullState ? !info.LastStable : info.LastStable) // if pullup, invert stable state.
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