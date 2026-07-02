/*
	beverly.hpp
	
	Button Events for AVR Easily (BEVERLY)
	Lightweight, efficient push button event handling for AVR microcontrollers


    Copyright (C) 2025-2026 Marcos Rubiano
	email:	markusianito@proton.me

    Licensed under BSD-2-clause. See LICENSE file.

*/



#pragma once
#include<Arduino.h>


#define DEBOUNCE_MS 20
#define LONGPRESS_MS 600

enum class BAction : uint8_t {
	Idle = 0,
	Pressed = 1,
	DoublePressed = 2,
	Held = 3,
	Released = 4,
	NotAvailable = 5
};

struct ButtonFlags
{
	uint8_t PullState : 1;
	uint8_t ButtonHeld : 1;
	uint8_t ButtonReady : 1;
	uint8_t LastStable : 1;
	uint8_t LastRaw : 1;
	uint8_t WasHeld : 1;
	uint8_t Unused : 2;

	ButtonFlags();
};

class Button {
	private:
	// timeout: ~65s
	uint16_t lastDebounceTime = 0;
    uint16_t pressTime = 0;
	ButtonFlags info; // bitmask
	uint8_t pin;
	public:
	BAction watch();
	uint8_t getPin();
	bool wasHeld();
	void discardHold();
	bool isReady();
	void start(uint8_t pull_state=INPUT);
	Button(uint8_t pin);
};