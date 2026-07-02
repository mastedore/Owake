
/*
    SolidCrystalI2C.h

	A library to make write-only i2c liquid crystal displays based on HD44780 easy to use,
	allowing user to keep track of the written text, write position,
	defined custom characters, etc.

    Copyright (c) 2025-2026 Mastedore <marcos@mastedore.com>

	This program is licensed under MIT license. See LICENSE file.


	
*/



#pragma once
#include <Arduino.h>

#define SOLIDCRYSTAL_I2C 2

#define lcd_bitmask_t uint32_t // type from stdint.h for main flags bitmask/bitfield
#define streaming_bitmask_t uint8_t // type from stdint.h for streaming flags

#define HD44780_MAX_RAM_BLOCK 40
#define HD44780_MAX_CGRAM 8

#define LEFT_TO_RIGHT true
#define RIGHT_TO_LEFT false
#define RIGHT LEFT_TO_RIGHT
#define LEFT RIGHT_TO_LEFT
#define instruction_register false
#define data_register true

#define SHIFT_CURSOR false
#define SHIFT_DISPLAY true



constexpr uint8_t CONTROL_CHAR_SKIP = 22;
constexpr uint8_t CONTROL_CHAR_END = 23;
constexpr uint8_t CONTROL_CHAR_EMPTY = 32;


bool isThereAnyLCD();

// lcd_flags lcd_bitmask_t (usually uint16_t) bitfield.
struct lcd_flags
{
	lcd_bitmask_t DisplayShift : 1;
	lcd_bitmask_t CursorDirection : 1;
	lcd_bitmask_t BlinkState : 1;
	lcd_bitmask_t CursorState : 1;
	lcd_bitmask_t DisplayState : 1;
	lcd_bitmask_t BacklightState : 1;
	lcd_bitmask_t FontSize : 1;
	lcd_bitmask_t Ready : 1;

	lcd_bitmask_t WriteRow : 2;
	lcd_bitmask_t WriteColumn : 6;

	lcd_bitmask_t ConstantCursorRow : 2;
	lcd_bitmask_t ConstantCursorColumn : 6;
	lcd_bitmask_t ConstantCursor : 1;

	lcd_bitmask_t WriteLayer : 1;
	lcd_bitmask_t WriteMode : 2;

	lcd_bitmask_t Unused : 4;

	lcd_flags();
};

struct stream_flags
{
	streaming_bitmask_t UseOverlay : 1;
	streaming_bitmask_t UserEnd : 1;
	streaming_bitmask_t OverlayEnd : 1;
	streaming_bitmask_t Unused : 5;

	void reset();
	stream_flags();
};

enum writeMode : uint8_t{
	raw = 0,
	text = 1,
	bin = 2,
	hex = 3
};

enum writeLayer : uint8_t{
	user = 0, overlay = 1
};

enum class cchar : uint8_t{
	endl = CONTROL_CHAR_END,
	skip = CONTROL_CHAR_SKIP,
	empty = CONTROL_CHAR_EMPTY
};

class LCD
{
	private:
	const uint8_t address;
	const uint8_t columns;
	const uint8_t rows;
	stream_flags streamingData;
	lcd_flags settings;
	uint8_t* buffer;
	uint8_t* overlayBuffer;
	uint8_t* dirtyCells;
	const uint8_t* glyphCache[HD44780_MAX_CGRAM];

	void sendi2c(uint8_t data);
	void sendNibble(uint8_t data, bool _register);
	void sendByte(uint8_t data, bool _register);

	inline void updateDisplayControl();
	inline void updateInputMode();


	void bufwrt(uint8_t i, uint8_t v);
	void write(const char* str);
	void write(char s);
	void write(int32_t s);
	void writeitoa(int32_t, uint8_t);
	void writePositionMod(int8_t);

	public:

	uint8_t getAddress();
	uint8_t getColumns();
	uint8_t getRows();
	uint8_t getBacklight();

	uint8_t geti();
	uint8_t start();
	int8_t isGlyphDefined(const uint8_t (&bmp)[8]);
	int8_t defineGlyph(const uint8_t (&bmp)[8], uint8_t addr);
	void clearAll();
	void display(bool state=true);
	void backlight(bool state=true);
	void cursor(bool state=true);
	void blink(bool state=true);
	void textDirection(bool dir=RIGHT);
	void cursorPosition(uint8_t r, uint8_t c);
	void cursorPosition(uint8_t i);
	void constantCursor(bool state=true);
	void row(uint8_t r);
	void column(uint8_t c);
	void seti(uint8_t c);
	void home(uint8_t r=0);
	void nextRow(bool forward=true);
	void clear();
	void flush();


	void writePosition(uint8_t c=0, uint8_t r=0);
	void writeChar(uint8_t c);

	LCD& operator++();
	LCD& operator++(int);
	LCD& operator--();
	LCD& operator--(int);
	
	
	LCD& operator<<(char s);
	LCD& operator<<(const char* s);
	LCD& operator<<(int64_t n);
	LCD& operator<<(uint64_t n);
	LCD& operator<<(int32_t n);
	LCD& operator<<(uint32_t n);
	LCD& operator<<(int16_t n);
	LCD& operator<<(uint16_t n);
	LCD& operator<<(uint8_t n);
	LCD& operator<<(writeMode m);
	LCD& operator<<(writeLayer ldiv);
	LCD& operator<<(cchar c);
	LCD& operator<<(const uint8_t (&bmp)[8]);
	



	LCD(uint8_t _address, uint8_t _cols, uint8_t _rows, bool useOverlay=false, bool _fontSize=0);
	~LCD();
	LCD(const LCD&);
	LCD& operator=(LCD _);
};


// at least 1 lcd MUST be created including this header.
extern LCD lcd;