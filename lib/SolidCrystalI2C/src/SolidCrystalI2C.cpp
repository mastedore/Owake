/*
    SolidCrystalI2C.cpp



    Copyright (c) 2025-2026 Mastedore <marcos@mastedore.com>

	This program is licensed under MIT license. See LICENSE file.
*/

#include <Arduino.h>
#include <Wire.h>
#include "SolidCrystalI2C.h"
#include "Facebits.h"
#include "fault.h"



// narrow conversion
template<typename T>
constexpr uint8_t tou8(T v)
{
    return static_cast<uint8_t>(v);
}



#if defined(__AVR__)
	#include <avr/pgmspace.h>
	#define READ_CONST_I8(i) pgm_read_byte(i)
#else
	#define READ_CONST_I8(i) *(i)
#endif

#define writeThenGet(a, s) static_cast<uint8_t>(writeDataAndGet<sci2c_bitmask_t, a>(settings, s));
#define shiftInstruction(sc, rl) LCD_INSTRUCT_SHIFT | (sc << 3) | (rl << 2)
#define BYTES_FOR_BUFFER tou8(((rows * columns) + 7)/8)




constexpr uint8_t STREAM_FLAG_END_OVERLAY =	0b00000001;
constexpr uint8_t STREAM_FLAG_END_USER = 	0b00000010;
constexpr uint8_t STREAM_FLAG_USE_OVERLAY = 0b10000000;
constexpr uint8_t STREAM_FLAG_DEFAULT = 	0b00000000;


constexpr uint8_t PCF_REGISTER_BIT =		0b00000001;
constexpr uint8_t PCF_READWRITE_BIT = 		0b00000010;
constexpr uint8_t PCF_ENABLE_BIT = 			0b00000100;
constexpr uint8_t PCF_BACKLIGHT_BIT = 		0b00001000;
constexpr uint8_t PCF_DATA_SHIFT = 4;




constexpr uint8_t LCD_INSTRUCT_CLEAR					= 0b00000001;
constexpr uint8_t LCD_INSTRUCT_HOME						= 0b00000010;
constexpr uint8_t LCD_INSTRUCT_INPUTMODE				= 0b00000100;
constexpr uint8_t LCD_INSTRUCT_DISPLAYCONTROL			= 0b00001000;
constexpr uint8_t LCD_INSTRUCT_SHIFT					= 0b00010000;
constexpr uint8_t LCD_INSTRUCT_CGRAM_ADDRESS			= 0b01000000;
constexpr uint8_t LCD_INSTRUCT_DDRAM_ADDRESS			= 0b10000000;

constexpr uint8_t LCD_INSTRUCT_INPUTMODE_MAXVALUE		= 0b00000111;
constexpr uint8_t LCD_INSTRUCT_DISPLAYCONTROL_MAXVALUE	= 0b00001111;
constexpr uint8_t LCD_INSTRUCT_CGRAM_ADDRESS_MAXVALUE	= 0b01111111; // different max
constexpr uint8_t LCD_INSTRUCT_DDRAM_ADDRESS_MAXVALUE	= 0b11111111; // different max

constexpr uint8_t LCD_INSTRUCT_8BITS_MODE				= 0b00000011;
constexpr uint8_t LCD_INSTRUCT_4BITS_MODE				= 0b00000010;
constexpr uint8_t LCD_INSTRUCT_5X8_MODE					= 0b00101000;
constexpr uint8_t LCD_INSTRUCT_5X10_MODE				= 0b00100000;
constexpr uint8_t LCD_INSTRUCT_1LINE_MODE				= 0b00100100;
constexpr uint8_t LCD_INSTRUCT_2LINES_MODE				= 0b00100000;



// Memory offsets per row (aka line). (Huang, 2005, pp. 327-328).
const uint8_t DDRAM_OFFSET[4] = {0x00, 0x40, 0x14, 0x54};


static bool LCD_READY = false;
bool isThereAnyLCD()
{
	return LCD_READY;
}


lcd_flags::lcd_flags() :
	DisplayShift(0),
	CursorDirection(LEFT_TO_RIGHT),
	BlinkState(0),
	CursorState(0),
	DisplayState(0),
	BacklightState(0),
	FontSize(0),
	Ready(0),
	WriteRow(0),
	WriteColumn(0),
	ConstantCursorRow(0),
	ConstantCursorColumn(0),
	ConstantCursor(0),
	Unused(0)
{

}

stream_flags::stream_flags() :
	UseOverlay(0),
	UserEnd(0),
	OverlayEnd(0),
	Unused(0)
{

}

void stream_flags::reset()
{
	OverlayEnd = 0;
	UserEnd = 0;
}


/* ███████████████████████████████████ 1st Layer/I²C Commands ███████████████████████████████████ */


void LCD::sendi2c(uint8_t data)
{
	data |= (settings.BacklightState) ? PCF_BACKLIGHT_BIT : 0;
	Wire.beginTransmission(address);
	Wire.write(data);
	Wire.endTransmission();
}

void LCD::sendNibble(uint8_t data, bool r)
{
	data <<= PCF_DATA_SHIFT;
	data |=  r;
	sendi2c(data | PCF_ENABLE_BIT);
	delayMicroseconds(1);
	sendi2c(tou8(data & (~PCF_ENABLE_BIT)));
	delayMicroseconds(50);
}

void LCD::sendByte(uint8_t bytee, bool rs)
{
	sendNibble((bytee >> 4), rs);
	sendNibble((bytee & 0xf), rs);
}


/* ███████████████████████████████████ 2nd Layer - Instructions ███████████████████████████████████ */


void LCD::clearAll()
{
	sendByte(LCD_INSTRUCT_CLEAR, instruction_register);
	delayMicroseconds(1640);
}

inline void LCD::updateDisplayControl()
{
	uint8_t ctrl = LCD_INSTRUCT_DISPLAYCONTROL;
	ctrl |= settings.BlinkState;
	ctrl |= settings.CursorState << 1;
	ctrl |= settings.DisplayState << 2;

	sendByte(ctrl, instruction_register);
}

inline void LCD::updateInputMode()
{
	uint8_t ctrl = LCD_INSTRUCT_INPUTMODE;
	ctrl |= settings.CursorDirection << 1;
	ctrl |= settings.DisplayShift;
	
	sendByte(ctrl, instruction_register);
}

void LCD::display(bool st)
{
	settings.DisplayState = st;
	updateDisplayControl();
}

void LCD::cursor(bool st)
{
	settings.CursorState = st;
	updateDisplayControl();
}

void LCD::blink(bool st)
{
	settings.BlinkState = st;
	updateDisplayControl();
}

void LCD::backlight(bool st)
{
	if (settings.BacklightState != st)
	{
		settings.BacklightState = st;

		sendi2c(st ? PCF_BACKLIGHT_BIT : 0);
	}
}

void LCD::textDirection(bool dir)
{
	settings.CursorDirection = dir;
	updateInputMode();
}

void LCD::cursorPosition(uint8_t c, uint8_t r)
{
	constantCursor(true);
	writePosition(c, r);
	settings.ConstantCursorColumn = c & mask<6>();
	settings.ConstantCursorRow = r & mask<2>();
}

void LCD::cursorPosition(uint8_t i)
{
	uint8_t r = i / columns;
	i = i % columns;

	cursorPosition(i, r);
}

void LCD::constantCursor(bool state)
{
	settings.ConstantCursor = state;
	cursor(state);
}

void LCD::writePosition(uint8_t c, uint8_t r)
{
	c %= HD44780_MAX_RAM_BLOCK;
	r %= rows;
	sendByte(LCD_INSTRUCT_DDRAM_ADDRESS | (c + DDRAM_OFFSET[r]), instruction_register);
}



/* ██████████████████████████████████████ 3rd Layer - API ██████████████████████████████████████ */


LCD& LCD::operator++()
{
	sendByte(shiftInstruction(SHIFT_DISPLAY, RIGHT), instruction_register);
	return *this;
}
LCD& LCD::operator++(int)
{
	sendByte(shiftInstruction(SHIFT_CURSOR, RIGHT), instruction_register);
	return *this;
}

LCD& LCD::operator--()
{
	sendByte(shiftInstruction(SHIFT_DISPLAY, LEFT), instruction_register);
	return *this;
}
LCD& LCD::operator--(int)
{
	sendByte(shiftInstruction(SHIFT_CURSOR, LEFT), instruction_register);
	return *this;
}


void LCD::row(uint8_t r)
{
	settings.WriteRow = r & mask<2>();
}

void LCD::column(uint8_t c)
{
	settings.WriteColumn = c & mask<6>();
}

void LCD::seti(uint8_t i)
{
	row(tou8(i / columns));
	column(tou8(i % columns));
}

void LCD::home(uint8_t r) // r=0 def
{
	row(r);
	column(0);
}

void LCD::nextRow(bool forward)
{
	seti(forward ? (geti()+columns) : (geti()-columns));
}

int8_t LCD::isGlyphDefined(const uint8_t (&bitmap)[8])
{
	for (int8_t i = 0; i < HD44780_MAX_CGRAM; i++)
	{
		if (glyphCache[i] == &bitmap[0])
		{
			return i;
		}
	}
	return -1;
}

int8_t LCD::defineGlyph(const uint8_t (&bitmap)[8], uint8_t _address)
{
	int8_t def = isGlyphDefined(bitmap);
	if (def >= 0) {return def;}

	bool fontSize = settings.FontSize;
	_address %= tou8(fontSize ? 4 : 8);
	sendByte(LCD_INSTRUCT_CGRAM_ADDRESS | (_address << 3), instruction_register);
	for (uint8_t i = 0; i < (fontSize ? 10 : 8); i++)
	{
		sendByte(READ_CONST_I8(bitmap + i), data_register);
	}
	glyphCache[_address] = &bitmap[0];

	//sendByte(LCD_INSTRUCT_DDRAM_ADDRESS, instruction_register);

	return -1;
}

void LCD::writeChar(uint8_t character)
{
	sendByte(character, data_register);
}

uint8_t LCD::start()
{
	if (settings.Ready) {return 1;}
	while (micros() <= 55000)
	{
		// we need to wait at least 55ms after power on (40ms after vcc=2.7v, then 15ms after vcc=4.5v)
		// before sending commands (Hitachi, 1998, p. 46)
		// if program already reached 55ms+, this wont wait
	}

	Wire.begin();
	backlight(true);
	
	sendNibble(0x03, instruction_register);
	delayMicroseconds(5000);
	sendNibble(0x03, instruction_register);
	delayMicroseconds(150);
	sendNibble(0x03, instruction_register);
	sendNibble(0x02, instruction_register);

	uint8_t functionSet = LCD_INSTRUCT_4BITS_MODE;
	functionSet |= (rows > 1) ? LCD_INSTRUCT_2LINES_MODE : LCD_INSTRUCT_1LINE_MODE;
	functionSet |= settings.FontSize ? LCD_INSTRUCT_5X10_MODE : LCD_INSTRUCT_5X8_MODE;

	sendByte(functionSet, instruction_register);

	cursor(false);
	display(true);
	clearAll();
	textDirection(LEFT_TO_RIGHT);

	
	for (uint8_t i = 0; i < HD44780_MAX_CGRAM; i++)
	{
		glyphCache[i] = nullptr;
	}

	settings.Ready = true;
	LCD_READY = true;
	
	return 0;
}

void LCD::clear()
{
	memset(buffer, CONTROL_CHAR_EMPTY, rows * columns);
	memset(dirtyCells, 0, BYTES_FOR_BUFFER);
	if (streamingData.UseOverlay)
	{
		memset(overlayBuffer, CONTROL_CHAR_SKIP, rows * columns);
	}
}






void LCD::writePositionMod(int8_t delta)
{
	delta = settings.CursorDirection ? delta : (-delta);
	if ((settings.WriteColumn + delta) >= columns)
	{
		row(tou8((settings.WriteRow + 1) % rows));
		column(tou8((settings.WriteColumn + delta) % columns));
	}
	else
	{
		column(tou8(settings.WriteColumn + delta));
	}
}

uint8_t LCD::geti()
{
	return tou8((settings.WriteRow * columns) + settings.WriteColumn);
}

void LCD::bufwrt(uint8_t index, uint8_t data)
{
	index %= tou8(rows * columns);
	uint8_t *workingBuff;
	switch (settings.WriteLayer)
	{
	case writeLayer::user:
		workingBuff = buffer;
		break;

	case writeLayer::overlay:
		if (overlayBuffer == nullptr) {return;}
		workingBuff = overlayBuffer;
		break;

	default:
		return;
	}
	*(workingBuff + index) = data;

	// So dirtyCells is an uint8_t[] that has enough bytes to
	// represent every cell in LCD (4 bytes for 16x2 LCD = 32 bits = 32 cells)

	// index >> 3 divides the index by 8 (using a right bit-shift), giving the byte in the array that contains the bit for this cell.
	// index & 7 gets the remainder when dividing by 8 (using a bitwise AND with 7), giving the bit position within that byte.
	// (1 << (index & 7)) creates a mask with a single bit set at the correct position.
	// |= sets that bit in the appropriate byte, marking the cell as dirty.
	dirtyCells[index >> 3] |= (1 << (index & 7));
}

void LCD::writeitoa(int32_t thing, uint8_t base)
{
	char bufferTmp[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	ltoa(thing, bufferTmp, base);
	for (char* ptr = bufferTmp; *ptr != 0; ++ptr)
	{
		bufwrt(geti(), tou8(*ptr));
		writePositionMod(1); // increment cursor position by 1
	}
}

void LCD::write(const char *str)
{
	while (*str != 0)
	{
		write(*str); // calls LCD::write(char s) which calls bufwrt;
		str++;
	}
}

void LCD::write(char s)
{
	writePositionMod(0);
	bufwrt(geti(), tou8(s));
	writePositionMod(1);
}

void LCD::write(int32_t s)
{
	writePositionMod(0);
	switch (settings.WriteMode)
	{
	case writeMode::text:
		writeitoa(s, 10);
		break;

	case writeMode::raw:
		bufwrt(geti(), uint8_t(s));
		writePositionMod(1);
		break;

	case writeMode::hex:
		writeitoa(s, 16);
		break;

	case writeMode::bin:
		writeitoa(s, 2);
		break;

	default:
		break;
	}
}

void LCD::flush()
{
	for (uint8_t i = 0; i < rows; i++)
	{
		streamingData.reset();

		for (uint8_t j = 0; j < columns; j++)
		{
			uint8_t index = tou8((i * columns) + j);
			
			if (streamingData.OverlayEnd)
			{
				writePosition(j, i);
				writeChar(CONTROL_CHAR_EMPTY);
				continue;
			}

			if (((dirtyCells[index >> 3] & (1 << (index & 7))) == 0) && (!streamingData.UserEnd))
			{
				continue;
			}

			switch (streamingData.UseOverlay ? overlayBuffer[index] : CONTROL_CHAR_SKIP)
			{
			case CONTROL_CHAR_END:
				streamingData.OverlayEnd = true;
				writePosition(j, i);
				writeChar(CONTROL_CHAR_EMPTY);
				break;

			case CONTROL_CHAR_SKIP:
			{

				if (streamingData.UserEnd)
				{
					writePosition(j, i);
					writeChar(CONTROL_CHAR_EMPTY);
					continue;
				}

				switch (buffer[index])
				{
				case CONTROL_CHAR_END:
					streamingData.UserEnd = true;
					writePosition(j, i);
					writeChar(CONTROL_CHAR_EMPTY);

					break;
				case CONTROL_CHAR_SKIP:
					break;

				default:
					writePosition(j, i);
					writeChar(buffer[index]);
					break;
				}
			}
			break;

			default:
					writePosition(j, i);
					writeChar(overlayBuffer[index]);
				break;
			}
		}
	}
	if (settings.ConstantCursor)
	{
		writePosition(settings.ConstantCursorColumn, settings.ConstantCursorRow);
	}
	memset(dirtyCells, 0, BYTES_FOR_BUFFER);
}





LCD &LCD::operator<<(char s)
{
	write(s);
	return *this;
}
LCD &LCD::operator<<(const char *s)
{
	write(s);
	return *this;
}


LCD &LCD::operator<<(int32_t n)
{
	write(n);
	return *this;
}
LCD &LCD::operator<<(uint32_t n)
{
	return *this << static_cast<int32_t>(n);
}
LCD &LCD::operator<<(uint16_t n)
{
	return *this << static_cast<int32_t>(n);
}
LCD &LCD::operator<<(int16_t n)
{
	return *this << static_cast<int32_t>(n);
}
LCD &LCD::operator<<(uint8_t n)
{
	return *this << static_cast<int32_t>(n);
}

//// WARNING: support for 64bit ints needed
LCD &LCD::operator<<(uint64_t n)
{
	return *this << static_cast<int32_t>(n);
}
//// WARNING: support for 64bit ints needed
LCD &LCD::operator<<(int64_t n)
{
	return *this << static_cast<int32_t>(n);
}


LCD &LCD::operator<<(writeMode m)
{
	settings.WriteMode = m & mask<2>();
	return *this;
}
LCD &LCD::operator<<(writeLayer m)
{
	if ((m == writeLayer::overlay) && (!streamingData.UseOverlay))
	{
		return *this;
	}
	settings.WriteLayer = m & 1;
	return *this;
}
LCD &LCD::operator<<(cchar c)
{
	write(static_cast<char>(c));
	return *this;
}

LCD& LCD::operator<<(const uint8_t (&bmp)[8])
{
	char addr = static_cast<char>(isGlyphDefined(bmp));
	if (addr >= 0)
	{
		write(addr);
	}
	else
	{
		write('*');
	}
	return *this;
}





/* ██████████████████████████████████████ Getters & Setters ██████████████████████████████████████ */


uint8_t LCD::getAddress()
{
	return address;
}
uint8_t LCD::getColumns()
{
	return columns;
}
uint8_t LCD::getRows()
{
	return rows;
}
uint8_t LCD::getBacklight()
{
	return settings.BacklightState;
}



/* ██████████████████████████████████████ Operators ██████████████████████████████████████ */

LCD::LCD(uint8_t _address, uint8_t _cols, uint8_t _rows, bool useOverlay, bool _fontSize) :
	address(_address),
	columns(_cols),
	rows(_rows),
	buffer(new uint8_t[rows * columns]),
	overlayBuffer(useOverlay ? (new uint8_t[rows * columns]) : nullptr),
	dirtyCells(new uint8_t[BYTES_FOR_BUFFER])
{
	streamingData.UseOverlay = useOverlay;
	if (_rows == 1)
	{
		settings.FontSize = _fontSize;
	}

	clear();
}

LCD::~LCD()
{
	FAULT(ILLEGAL_OPERATION);
}
LCD& LCD::operator=(LCD _)
{
	FAULT(ILLEGAL_OPERATION);
}
LCD::LCD(const LCD& _): address(0), columns(0), rows(0)
{
	FAULT(ILLEGAL_OPERATION);
}





















/*

void LCD::cursor(bool enabled)
{
	renderCursor(enabled);
	settings.CursorEnabled = enabled;
}
void LCD::cursorPosition(uint8_t _colbegin, uint8_t _colend, uint8_t _row)
{
	settings.CursorColBegin = _colbegin;
	settings.CursorColEnd = _colend;
	settings.CursorRow = _row;
}

void LCD::renderCursor(bool state)
{
	if (settings.CursorEnabled)
	{
		writePosition(settings.CursorColBegin, settings.CursorRow);
		for (uint8_t i = settings.CursorColBegin; i <= settings.CursorColEnd; i++)
		{
			if (state)
			{
				writeChar(cursorChar);
			}
			else
			{
				uint8_t index = (settings.CursorRow * cols) + i;
				writeChar(
					(overlayBuffer[index] == CONTROL_CHAR_SKIP) ?
					buffer[index] :
					overlayBuffer[index]
				);
			}
		}
	}
}

// refresh does not render buffer to lcd, it refreshes settings and should be called forever in loop()
void LCD::refresh()
{
	uint32_t now = millis();
	if (settings.CursorEnabled)
	{
		switch (settings.CursorDisplaying)
		{
		case false:
			if (millis() - cursorTimestamp >= cursorOffMs)
			{
				settings.CursorDisplaying = true;
				cursorTimestamp = now;
				renderCursor(true);
				flush();
			}
			break;

		case true:
			if (now - cursorTimestamp >= cursorOnMs)
			{
				settings.CursorDisplaying = false;
				cursorTimestamp = now;
				renderCursor(false);
				flush();
			}
			break;
		}
	}
}

*/