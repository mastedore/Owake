/*
	thislcd.cpp

	Implements custom functions and
	symbols for SolidCrystalI2C to use in the Owake project


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

#include "thislcd.hpp"
#include "main.hpp"

// ASCII codes of the separators. They go through printThicc() in place of a
// digit, which prints them as-is in font 0 and as blank cells in the tall fonts.
constexpr uint8_t dot = 46;
constexpr uint8_t colon = 58;
constexpr uint8_t slash = 47;

// CGRAM slots for fonts 1 and 2. Both fonts load into the same slots, so one
// table (glyphs_font_1_2) works for either. _0 is a top half, _1 a bottom half.
#define ZERO_0 0
#define ZERO_1 1
#define ONE_1 2
#define TWO_1 3
#define THREE_1 4
#define FIVE_0 5
#define SEVEN_0 6
#define EIGHT_1 7

// const uint8_t registered[] = 	{0x1f, 0x11, 0x15, 0x11, 0x13, 0x15, 0x1f, 0x00};


namespace Glyph
{
	const uint8_t left[8] PROGMEM = {0x01, 0x03, 0x07, 0x0f, 0x0f, 0x07, 0x03, 0x01};
	const uint8_t bar[8] PROGMEM = {0x00, 0x00, 0x00, 0x1f, 0x1f, 0x00, 0x00, 0x00};
	const uint8_t right[8] PROGMEM = {0x10, 0x18, 0x1c, 0x1e, 0x1e, 0x1c, 0x18, 0x10};
	const uint8_t hourglass[8] PROGMEM = {0x1f, 0x11, 0x0a, 0x04, 0x04, 0x0a, 0x11, 0x1f};
	const uint8_t clock[8] PROGMEM = {0x1b, 0x0e, 0x11, 0x15, 0x17, 0x11, 0x0e, 0x1b};
	const uint8_t homie[8] PROGMEM = {0x04, 0x0a, 0x11, 0x1f, 0x11, 0x15, 0x15, 0x1f};
	const uint8_t chrono[8] PROGMEM = {0x00, 0x04, 0x0e, 0x11, 0x15, 0x15, 0x11, 0x0e};
	const uint8_t man[8] PROGMEM = {0x11, 0x15, 0x1f, 0x0e, 0x0e, 0x0a, 0x0a, 0x1b};
}

namespace Font0
{
	const uint8_t Owake[8] PROGMEM =		{0x0E, 0x1f, 0x1b, 0x1b, 0x19, 0x1f, 0x1f, 0x0e};
	const uint8_t oWake[8] PROGMEM = 	{0x00, 0x11, 0x1b, 0x1b, 0x1b, 0x1f, 0x1f, 0x0a};
	const uint8_t owAke[8] PROGMEM = 	{0x0e, 0x1f, 0x1b, 0x1b, 0x19, 0x1f, 0x1f, 0x1b};
	const uint8_t owaKe[8] PROGMEM = 	{0x1b, 0x1f, 0x1e, 0x1c, 0x1e, 0x1f, 0x1f, 0x1b};
	const uint8_t owakE[8] PROGMEM = 	{0x0f, 0x1e, 0x18, 0x1c, 0x1c, 0x18, 0x1e, 0x0f};
	const uint8_t copyright[8] PROGMEM =	{0x1f, 0x11, 0x15, 0x17, 0x15, 0x11, 0x1f, 0x00};
	const uint8_t e[8] PROGMEM = 	{0x00, 0x00, 0x0A, 0x1f, 0x0a, 0x00, 0x00, 0x00};
	const uint8_t wink[8] PROGMEM = 		{0x00, 0x09, 0x05, 0x09, 0x00, 0x10, 0x11, 0x0e};
}

namespace Font1
{
	const uint8_t zero_0[8] PROGMEM = {0x04, 0x0e, 0x1f, 0x1b, 0x1b, 0x1b, 0x1b, 0x1b};
	const uint8_t zero_1[8] PROGMEM = {0x1b, 0x1b, 0x1b, 0x1b, 0x1b, 0x1b, 0x0f, 0x0f};
	const uint8_t one_1[8] PROGMEM = {0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03};
	const uint8_t two_1[8] PROGMEM = {0x03, 0x07, 0x0e, 0x1c, 0x18, 0x18, 0x1f, 0x1f};
	const uint8_t three_1[8] PROGMEM = {0x03, 0x0f, 0x0f, 0x03, 0x03, 0x1b, 0x1f, 0x0f};
	const uint8_t five_0[8] PROGMEM = {0x1f, 0x1f, 0x18, 0x18, 0x1c, 0x1e, 0x07, 0x03};
	const uint8_t seven_0[8] PROGMEM = {0x1f, 0x1f, 0x03, 0x03, 0x03, 0x03, 0x03, 0x0f};
	const uint8_t eight_1[8] PROGMEM = {0x0e, 0x1f, 0x1f, 0x1b, 0x1b, 0x1b, 0x1f, 0x0f};
}

namespace Font2
{
	const uint8_t zero_0[8] PROGMEM = {0x0e, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a};
	const uint8_t zero_1[8] PROGMEM = {0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0e};
	const uint8_t one_1[8] PROGMEM = {0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02};
	const uint8_t two_1[8] PROGMEM = {0x02, 0x02, 0x06, 0x0c, 0x08, 0x08, 0x08, 0x0e};
	const uint8_t three_1[8] PROGMEM = {0x02, 0x0e, 0x02, 0x02, 0x02, 0x0a, 0x0a, 0x0e};
	const uint8_t five_0[8] PROGMEM = {0x0e, 0x08, 0x08, 0x08, 0x08, 0x0E, 0x02, 0x02};
	const uint8_t seven_0[8] PROGMEM = {0x00, 0x1e, 0x02, 0x02, 0x02, 0x02, 0x02, 0x0e};
	const uint8_t eight_1[8] PROGMEM = {0x0e, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0e};
}

// Digits 0-9 as two stacked cells: low byte on top, high byte below. With only
// 8 CGRAM slots the halves are shared, e.g. 0, 2 and 3 all use ZERO_0 on top.
const ThickGlyph1x2 glyphs_font_1_2[] PROGMEM = {
	makeThicc(ZERO_1, ZERO_0),
	makeThicc(ONE_1, ONE_1),
	makeThicc(TWO_1, ZERO_0),
	makeThicc(THREE_1, ZERO_0),
	makeThicc(ONE_1, ZERO_1),
	makeThicc(ZERO_1, FIVE_0),
	makeThicc(EIGHT_1, TWO_1),
	makeThicc(ONE_1, SEVEN_0),
	makeThicc(EIGHT_1, EIGHT_1),
	makeThicc(ONE_1, EIGHT_1)};

void defineHomeSymbols()
{
	// defineGlyph() returns 0 only when Glyph::left is already cached in slot 0.
	// That means the home set is still loaded and the other 7 uploads can be skipped.
	if (lcd.defineGlyph(Glyph::left, 0))
	{
		lcd.defineGlyph(Glyph::bar, 1);
		lcd.defineGlyph(Glyph::right, 2);
		lcd.defineGlyph(Glyph::hourglass, 3);
		lcd.defineGlyph(Glyph::clock, 4);
		lcd.defineGlyph(Glyph::homie, 5);
		lcd.defineGlyph(Glyph::chrono, 6);
		lcd.defineGlyph(Glyph::man, 7);
	}
}

void defineFont(uint8_t font)
{
	switch (font)
	{
	case 0:
		lcd.defineGlyph(Font0::Owake, 0);
		lcd.defineGlyph(Font0::oWake, 1);
		lcd.defineGlyph(Font0::owAke, 2);
		lcd.defineGlyph(Font0::owaKe, 3);
		lcd.defineGlyph(Font0::owakE, 4);
		lcd.defineGlyph(Font0::copyright, 5);
		lcd.defineGlyph(Font0::e, 6);
		lcd.defineGlyph(Font0::wink, 7);
		break;
	case 1:
		lcd.defineGlyph(Font1::zero_0, 0);
		lcd.defineGlyph(Font1::zero_1, 1);
		lcd.defineGlyph(Font1::one_1, 2);
		lcd.defineGlyph(Font1::two_1, 3);
		lcd.defineGlyph(Font1::three_1, 4);
		lcd.defineGlyph(Font1::five_0, 5);
		lcd.defineGlyph(Font1::seven_0, 6);
		lcd.defineGlyph(Font1::eight_1, 7);
		break;

	case 2:
		lcd.defineGlyph(Font2::zero_0, 0);
		lcd.defineGlyph(Font2::zero_1, 1);
		lcd.defineGlyph(Font2::one_1, 2);
		lcd.defineGlyph(Font2::two_1, 3);
		lcd.defineGlyph(Font2::three_1, 4);
		lcd.defineGlyph(Font2::five_0, 5);
		lcd.defineGlyph(Font2::seven_0, 6);
		lcd.defineGlyph(Font2::eight_1, 7);
		break;

	default:
		break;
	}
}

// The arrows live on the overlay layer, so menu text written underneath
// doesn't erase them. Writing skip gives the cell back to the user layer.
void toggleArrows(bool on)
{
	lcd << writeLayer::overlay;
	if (on)
	{
		defineHomeSymbols();
		lcd.home(1);
		lcd << Glyph::left;
		lcd.column(static_cast<uint8_t>(COLS - 1));
		lcd << Glyph::right;
	}
	else
	{
		lcd.home(1);
		lcd << cchar::skip;
		lcd.column(static_cast<uint8_t>(COLS - 1));
		lcd << cchar::skip;
	}
	lcd << writeLayer::user;
}

// Prints mm:ss.mmm. Minutes aren't capped, so past 99 only the last two
// digits fit in the tall fonts.
void insertMillis(uint32_t time, bool thicc, uint8_t font)
{
	uint32_t ms = time % 1000U;
	uint32_t totalSeconds = time / 1000U;
	uint8_t sec = static_cast<uint8_t>(totalSeconds % 60U);
	uint8_t min = static_cast<uint8_t>(totalSeconds / 60U);

	uint8_t min0 = min % 10;
	uint8_t sec0 = sec % 10;
	uint8_t ms0 = static_cast<uint8_t>(ms % 10U);
	uint8_t ms1 = static_cast<uint8_t>((ms / 10U) % 10U);
	uint8_t ms2 = static_cast<uint8_t>((ms / 100U) % 10U);

	min = min / 10;
	sec = sec / 10;

	printThicc(min, font);
	printThicc(min0, font);
	printThicc(colon, font);
	printThicc(sec, font);
	printThicc(sec0, font);
	printThicc(dot, font);
	printThicc(ms2, font);
	printThicc(ms1, font);
	printThicc(ms0, font);
}

void insertTime(uint32_t &time, bool seconds, bool thicc, uint8_t font)
{
	PackedTime tyme(time);
	uint8_t h0 = tyme.hour % 10;
	uint8_t m0 = tyme.minute % 10;
	uint8_t h = tyme.hour / 10;
	uint8_t m = tyme.minute / 10;

	printThicc(h, font);
	printThicc(h0, font);
	printThicc(colon, font);
	printThicc(m, font);
	printThicc(m0, font);

	if (seconds)
	{
		uint8_t s = tyme.second;
		uint8_t s0 = s % 10;
		s = s / 10;
		printThicc(colon, font);
		printThicc(s, font);
		printThicc(s0, font);
	}
}

void insertDate(const PackedDate &time, bool thicc, uint8_t font)
{

	printThicc(time.day / 10, font);
	printThicc(time.day % 10, font);
	printThicc(slash, font);

	printThicc(time.month / 10, font);
	printThicc(time.month % 10, font);
	printThicc(slash, font);

	const uint16_t year = static_cast<uint16_t>(time.year + EPOCH);
	printThicc((year / 1000) % 10, font);
	printThicc((year / 100) % 10, font);
	printThicc((year / 10) % 10, font);
	printThicc(year % 10, font);
}

static uint8_t position = 0;
void printThicc(uint8_t digit, uint8_t font)
{
	bool nofont = font == 0;
	bool space = false;
	uint16_t dig;

	if (digit == dot || digit == slash || digit == colon)
	{

		if (nofont)
		{
			lcd << static_cast<char>(digit);
			return;
		}

		space = true;
	}
	else
	{
		if (nofont)
		{
			lcd << static_cast<int32_t>(digit);
			return;
		}
	}

	if (!space)
	{

		digit %= 10;
		switch (font)
		{
		case 1:
		case 2:
			dig = pgm_read_word(&glyphs_font_1_2[digit]);
			break;
		case 255:
			dig = static_cast<uint16_t>(static_cast<uint16_t>(digit) << 8U) | static_cast<uint16_t>(digit);
			break;

		default:
			lcd << digit;
			return;
		}
	}

	// Top half at the current cell, bottom half right below it (the first write
	// already moved one cell right, hence columns - 1), then continue on the
	// top row one cell to the right. Assumes the digit starts on row 0.
	position = static_cast<uint8_t>(lcd.geti());
	lcd << (space ? ' ' : static_cast<char>(lowByte(dig)));
	lcd.seti(static_cast<uint8_t>(lcd.geti() + lcd.getColumns() - 1));
	lcd << (space ? ' ' : static_cast<char>(highByte(dig)));
	lcd.seti(static_cast<uint8_t>(position + 1));
}