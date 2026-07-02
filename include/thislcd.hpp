/*
	thislcd.hpp
	
	Declares custom functions for SolidCrystalI2C
	custom symbols are defined in thislcd.CPP



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
    along with this program.  If not, see <https://www.gnu.org/licenses/>
*/

#ifndef THIS_LCD_HPP
#define THIS_LCD_HPP
#include "time.hpp"
#include "SolidCrystalI2C.h"
#include "main.hpp"

constexpr uint8_t DIGIT_FONTS = 2; 

using ThickGlyph1x2 = uint16_t;

constexpr ThickGlyph1x2 makeThicc(uint8_t y1, uint8_t y0) {
    return (static_cast<ThickGlyph1x2>(y1) << 8) | y0;
}


namespace Glyph
{
	extern const uint8_t left		[8] PROGMEM;
	extern const uint8_t bar		[8] PROGMEM;
	extern const uint8_t right		[8] PROGMEM;
	extern const uint8_t hourglass	[8] PROGMEM;
	extern const uint8_t clock		[8] PROGMEM;
	extern const uint8_t homie		[8] PROGMEM;
	extern const uint8_t chrono		[8] PROGMEM;
	extern const uint8_t man		[8] PROGMEM;
}

namespace Font1
{
	extern const uint8_t zero_0		[8] PROGMEM;
	extern const uint8_t zero_1		[8] PROGMEM;
	extern const uint8_t one_1		[8] PROGMEM;
	extern const uint8_t two_1		[8] PROGMEM;
	extern const uint8_t three_1	[8] PROGMEM;
	extern const uint8_t five_0		[8] PROGMEM;
	extern const uint8_t seven_0	[8] PROGMEM;
	extern const uint8_t eight_0	[8] PROGMEM;
}

namespace Font2
{
	extern const uint8_t zero_0		[8] PROGMEM;
	extern const uint8_t zero_1		[8] PROGMEM;
	extern const uint8_t one_1		[8] PROGMEM;
	extern const uint8_t two_1		[8] PROGMEM;
	extern const uint8_t three_1	[8] PROGMEM;
	extern const uint8_t five_0		[8] PROGMEM;
	extern const uint8_t seven_0	[8] PROGMEM;
	extern const uint8_t eight_0	[8] PROGMEM;
}

extern const ThickGlyph1x2 glyphs_font_1_2[] PROGMEM;


void defineHomeSymbols();
void defineFont(uint8_t font);
void toggleArrows(bool on);
void insertMillis(uint32_t time, bool thicc, uint8_t font);
void insertTime(uint32_t& time, bool seconds, bool thicc, uint8_t font);
void insertDate(const PackedDate& date, bool thicc, uint8_t font);
void printThicc(uint8_t digit, uint8_t font);


#endif // THIS_LCD_CPP