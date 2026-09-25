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



// Short name for the uint8_t casts this file is full of. Most of them come
// from bitfields and int math that would otherwise trip -Wconversion.
template<typename T>
constexpr uint8_t tou8(T v)
{
    return static_cast<uint8_t>(v);
}



// Glyph bitmaps live in flash on AVR, so they need pgm_read_byte(). On other
// chips const data is already addressable like normal memory.
#if defined(__AVR__)
	#include <avr/pgmspace.h>
	#define READ_CONST_I8(i) pgm_read_byte(i)
#else
	#define READ_CONST_I8(i) *(i)
#endif

#define writeThenGet(a, s) static_cast<uint8_t>(writeDataAndGet<sci2c_bitmask_t, a>(settings, s));
// Cursor/display shift instruction: bit 3 picks display (1) or cursor (0),
// bit 2 picks right (1) or left (0).
#define shiftInstruction(sc, rl) LCD_INSTRUCT_SHIFT | (sc << 3) | (rl << 2)
// One dirty bit per cell, rounded up to whole bytes (16x2 -> 4 bytes).
#define BYTES_FOR_BUFFER tou8(((rows * columns) + 7)/8)




constexpr uint8_t STREAM_FLAG_END_OVERLAY =	0b00000001;
constexpr uint8_t STREAM_FLAG_END_USER = 	0b00000010;
constexpr uint8_t STREAM_FLAG_USE_OVERLAY = 0b10000000;
constexpr uint8_t STREAM_FLAG_DEFAULT = 	0b00000000;


// Bits of the byte sent to the PCF8574. The low nibble is control lines,
// the high nibble carries the data nibble for the LCD.
constexpr uint8_t PCF_REGISTER_BIT =		0b00000001;
constexpr uint8_t PCF_READWRITE_BIT = 		0b00000010;
constexpr uint8_t PCF_ENABLE_BIT = 			0b00000100;
constexpr uint8_t PCF_BACKLIGHT_BIT = 		0b00001000;
constexpr uint8_t PCF_DATA_SHIFT = 4;




// HD44780 instruction opcodes. Each one is the highest set bit of the byte,
// and the bits below it are that instruction's options.
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

// Sent as lone nibbles during the power-on handshake that puts the controller
// in 4-bit mode (Hitachi, 1998, p. 46).
constexpr uint8_t LCD_INIT_NIBBLE_8BITS					= 0b00000011;
constexpr uint8_t LCD_INIT_NIBBLE_4BITS					= 0b00000010;

// Function set: 0 0 1 DL N F x x
constexpr uint8_t LCD_INSTRUCT_FUNCTIONSET				= 0b00100000;
constexpr uint8_t LCD_FUNCTION_8BITS					= 0b00010000; // DL, left at 0 (4-bit bus)
constexpr uint8_t LCD_FUNCTION_2LINES					= 0b00001000; // N
constexpr uint8_t LCD_FUNCTION_5X10						= 0b00000100; // F, only valid with N = 0



// Memory offsets per row (aka line). (Huang, 2005, pp. 327-328).
// Rows are not contiguous in DDRAM: on a 4-row display row 2 is really the
// continuation of row 0 (0x00 + 20), and row 3 continues row 1.
const uint8_t DDRAM_OFFSET[4] = {0x00, 0x40, 0x14, 0x54};


// Set once any LCD finishes start(). fault uses it to know whether it can
// print an error or should just halt.
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
	WriteLayer(writeLayer::user),
	WriteMode(writeMode::raw),
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

// Called at the start of every row in flush(), since an END only blanks
// the rest of the row it's on.
void stream_flags::reset()
{
	OverlayEnd = 0;
	UserEnd = 0;
}


/* ---------------- Layer 1: I2C transport ---------------- */


// The PCF8574 drives its pins with whatever byte it got last, so the
// backlight bit has to ride along on every single write.
void LCD::sendi2c(uint8_t data)
{
	data |= (settings.BacklightState) ? PCF_BACKLIGHT_BIT : 0;
	Wire.beginTransmission(address);
	Wire.write(data);
	Wire.endTransmission();
}

// Expander wiring: P0 = RS, P1 = RW, P2 = E, P3 = backlight, P4-P7 = D4-D7.
// The LCD latches on the falling edge of E, so every nibble costs two I2C
// writes: one with E high, one with E low.
void LCD::sendNibble(uint8_t data, bool r)
{
	data <<= PCF_DATA_SHIFT;
	data |=  r; // RS: 0 = instruction, 1 = data. RW stays 0, this library never reads.
	sendi2c(data | PCF_ENABLE_BIT);
	// The enable pulse needs ~450 ns. The I2C write alone takes longer than
	// that, so the delay is just a safety margin.
	delayMicroseconds(1);
	sendi2c(tou8(data & (~PCF_ENABLE_BIT)));
	delayMicroseconds(50); // most instructions take ~37 us to execute
}

// In 4-bit mode every byte goes out as two nibbles, high one first.
void LCD::sendByte(uint8_t bytee, bool rs)
{
	sendNibble((bytee >> 4), rs);
	sendNibble((bytee & 0xf), rs);
}


/* ---------------- Layer 2: HD44780 instructions ---------------- */


void LCD::clearAll()
{
	// Clear is one of the two slow instructions (with home), about 1.52 ms.
	sendByte(LCD_INSTRUCT_CLEAR, instruction_register);
	delayMicroseconds(1640);
}

// Display, cursor and blink share one instruction, so changing any of them
// resends all three from the cached settings.
inline void LCD::updateDisplayControl()
{
	uint8_t ctrl = LCD_INSTRUCT_DISPLAYCONTROL;
	ctrl |= settings.BlinkState;
	ctrl |= settings.CursorState << 1;
	ctrl |= settings.DisplayState << 2;

	sendByte(ctrl, instruction_register);
}

// Entry mode: which way the address moves after each write, and whether the
// whole display shifts along with it.
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
	// The backlight is a pin on the expander, not an LCD instruction. Any write
	// updates it, so an empty byte with only that bit is enough.
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

// A "constant" cursor gets put back on this cell at the end of every flush(),
// because flush() leaves the hardware address wherever it wrote last.
void LCD::cursorPosition(uint8_t c, uint8_t r)
{
	constantCursor(true);
	writePosition(c, r);
	settings.ConstantCursorColumn = c & mask<6>();
	settings.ConstantCursorRow = r & mask<2>();
}

// Same as above, but with a flat cell index (row * columns + column).
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

// Moves the hardware address right away, bypassing the buffers. Used by
// flush() and by code that writes straight to the glass (the credits).
void LCD::writePosition(uint8_t c, uint8_t r)
{
	c %= HD44780_MAX_RAM_BLOCK;
	r %= rows;
	sendByte(LCD_INSTRUCT_DDRAM_ADDRESS | (c + DDRAM_OFFSET[r]), instruction_register);
}



/* ---------------- Layer 3: buffered API ---------------- */


// Prefix ++/-- scroll the whole display, postfix ++/-- only move the cursor.
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


// row(), column(), seti() and home() only move the buffer write position.
// Nothing is sent to the LCD until the next flush().
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

// Same column, one row down or up. seti() wraps it around the buffer.
void LCD::nextRow(bool forward)
{
	seti(forward ? (geti()+columns) : (geti()-columns));
}

// Glyphs are matched by the address of their bitmap, not its contents, so two
// arrays with the same pixels still count as different glyphs.
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

// Returns the slot if the bitmap was already loaded (nothing is sent), or -1
// after uploading it to _address.
int8_t LCD::defineGlyph(const uint8_t (&bitmap)[8], uint8_t _address)
{
	int8_t def = isGlyphDefined(bitmap);
	if (def >= 0) {return def;}

	// 5x8 has 8 slots of 8 rows each. 5x10 has 4 slots that are 16 rows
	// apart in CGRAM, with 11 rows in use (the 11th is the cursor line).
	// The bitmap only carries 8 rows, so in 5x10 the last 3 go out blank.
	bool fontSize = settings.FontSize;
	_address %= tou8(fontSize ? 4 : 8);
	sendByte(tou8(LCD_INSTRUCT_CGRAM_ADDRESS | (_address << (fontSize ? 4 : 3))), instruction_register);
	for (uint8_t i = 0; i < (fontSize ? 11 : 8); i++)
	{
		sendByte((i < 8) ? READ_CONST_I8(bitmap + i) : tou8(0), data_register);
	}
	glyphCache[_address] = &bitmap[0];

	// The address counter is still pointing into CGRAM at this point. That is
	// fine because flush() and writePosition() always set a DDRAM address
	// before writing, but a bare writeChar() right now would corrupt a glyph.

	return -1;
}

// Raw data write at the current hardware address, no buffers involved.
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
	
	// Whatever state the controller woke up in (8-bit, or halfway through a
	// 4-bit byte after a reset), three "8-bit" nibbles resync it and the
	// fourth switches it to 4-bit.
	sendNibble(LCD_INIT_NIBBLE_8BITS, instruction_register);
	delayMicroseconds(5000);
	sendNibble(LCD_INIT_NIBBLE_8BITS, instruction_register);
	delayMicroseconds(150);
	sendNibble(LCD_INIT_NIBBLE_8BITS, instruction_register);
	sendNibble(LCD_INIT_NIBBLE_4BITS, instruction_register);

	uint8_t functionSet = LCD_INSTRUCT_FUNCTIONSET;
	if (rows > 1)
	{
		functionSet |= LCD_FUNCTION_2LINES;
	}
	else if (settings.FontSize)
	{
		functionSet |= LCD_FUNCTION_5X10;
	}

	sendByte(functionSet, instruction_register);

	// The controller comes up with the display off, so turn it on here.
	cursor(false);
	display(true);
	clearAll();
	textDirection(LEFT_TO_RIGHT);

	
	// CGRAM holds random data after power on, so nothing counts as loaded.
	for (uint8_t i = 0; i < HD44780_MAX_CGRAM; i++)
	{
		glyphCache[i] = nullptr;
	}

	settings.Ready = true;
	LCD_READY = true;
	
	return 0;
}

// Resets the buffers without marking anything dirty, so the glass keeps its
// old content until those cells get written again. Pair it with clearAll()
// when the screen really has to go blank right away.
void LCD::clear()
{
	memset(buffer, CONTROL_CHAR_EMPTY, rows * columns);
	memset(dirtyCells, 0, BYTES_FOR_BUFFER);
	if (streamingData.UseOverlay)
	{
		memset(overlayBuffer, CONTROL_CHAR_SKIP, rows * columns);
	}
}






// Moves the buffer write position by delta cells in the current text
// direction, wrapping to the next (or previous) row at the edges.
void LCD::writePositionMod(int8_t delta)
{
	delta = settings.CursorDirection ? delta : (-delta);
	int16_t col = static_cast<int16_t>(settings.WriteColumn) + delta;
	if (col >= columns)
	{
		row(tou8((settings.WriteRow + 1) % rows));
		column(tou8(col % columns));
	}
	else if (col < 0)
	{
		// Right-to-left text walking past column 0 continues at the end of the previous row.
		row(tou8((settings.WriteRow + rows - 1) % rows));
		column(tou8(col + columns));
	}
	else
	{
		column(tou8(col));
	}
}

// Write position as a flat index into the buffers.
uint8_t LCD::geti()
{
	return tou8((settings.WriteRow * columns) + settings.WriteColumn);
}

// The only place that touches the buffers. Writes go to whichever layer
// operator<<(writeLayer) selected, and the cell gets marked dirty.
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
	// Worst case is base 2: ltoa() prints non-decimal values as unsigned, so
	// that's 32 digits plus the terminator (the extra byte covers a '-').
	char bufferTmp[34] = {0};
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
	// A delta of 0 only normalizes the position: if column() was set past
	// the last column, this moves it to the next row before writing.
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

// Walks every cell and only sends the dirty ones. The overlay wins over the
// user layer unless it holds skip. An END (cchar::endl) blanks the rest of its
// row, dirty or not, so old text doesn't linger after it. After a user-layer
// END, overlay cells still show through.
void LCD::flush()
{
	for (uint8_t i = 0; i < rows; i++)
	{
		// END markers only last until the end of their row.
		streamingData.reset();

		for (uint8_t j = 0; j < columns; j++)
		{
			uint8_t index = tou8((i * columns) + j);
			
			// An overlay END blanks everything after it, whatever either layer holds.
			if (streamingData.OverlayEnd)
			{
				writePosition(j, i);
				writeChar(CONTROL_CHAR_EMPTY);
				continue;
			}

			// Clean cells are skipped, except after a user END, where the rest
			// of the row has to be blanked even if nothing changed there.
			if (((dirtyCells[index >> 3] & (1 << (index & 7))) == 0) && (!streamingData.UserEnd))
			{
				continue;
			}

			// Without an overlay buffer every cell behaves as if the overlay held skip.
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
				case CONTROL_CHAR_SKIP: // nothing written here, leave the glass alone
					break;

				default:
					writePosition(j, i);
					writeChar(buffer[index]);
					break;
				}
			}
			break;

			// The overlay holds a real character, it hides the user layer.
			default:
					writePosition(j, i);
					writeChar(overlayBuffer[index]);
				break;
			}
		}
	}
	// Writing moved the hardware address, and the visible cursor with it.
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
// Every integer type funnels into write(int32_t), which formats according
// to the current writeMode.
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
	// Without an overlay buffer, asking for it is ignored and writes keep
	// going to the user layer.
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
	int8_t slot = isGlyphDefined(bmp);
	if (slot >= 0)
	{
		// In 5x10 mode the character code bits 2-1 pick the slot and bit 0
		// is ignored, so slot n is shown with code 2n.
		write(static_cast<char>(settings.FontSize ? (slot << 1) : slot));
	}
	else
	{
		// Not loaded in CGRAM, so show a placeholder instead of a wrong glyph.
		write('*');
	}
	return *this;
}





/* ---------------- Getters ---------------- */


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



/* ---------------- Construction ---------------- */

LCD::LCD(uint8_t _address, uint8_t _cols, uint8_t _rows, bool useOverlay, bool _fontSize) :
	address(_address),
	columns(_cols),
	rows(_rows),
	// Allocated once at startup and never freed, the LCD lives for the whole program.
	buffer(new uint8_t[rows * columns]),
	overlayBuffer(useOverlay ? (new uint8_t[rows * columns]) : nullptr),
	dirtyCells(new uint8_t[BYTES_FOR_BUFFER])
{
	streamingData.UseOverlay = useOverlay;
	// The HD44780 only supports 5x10 characters in 1-line mode.
	if (_rows == 1)
	{
		settings.FontSize = _fontSize;
	}

	clear();
}

// A global LCD is never destroyed on AVR, so getting here means something
// went wrong (a temporary copy, a stack LCD going out of scope).
LCD::~LCD()
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