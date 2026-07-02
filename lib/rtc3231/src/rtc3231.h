/*
	rtc3231.h

	A simple lightweight driver library for the DS3231 RTC 

	Copyright (C) 2026 Marcos Rubiano
	email:	markusianito@proton.me

	This program is licensed under MIT license. See LICENSE file.
	
*/

#pragma once

#include<stdint.h>

#define btime_t uint32_t
#define rtcflags_t uint16_t

constexpr uint8_t RTC_I2C_ADDRESS = 0x68;
constexpr uint8_t RTC_REGISTER_TIME = 0x0;
constexpr uint8_t RTC_REGISTER_A1 = 0x7;
constexpr uint8_t RTC_REGISTER_A2 = 0xb;
constexpr uint8_t RTC_REGISTER_CONTROL = 0xe;
constexpr uint8_t RTC_REGISTER_STATUS = 0xf;
constexpr uint8_t RTC_REGISTER_TEMP = 0x11;
constexpr uint8_t RTC_REGISTER_TEMPDC = 0x12;



constexpr bool AM = false;
constexpr bool PM = true;

constexpr bool HMODE_24 = false;
constexpr bool HMODE_12 = true;

constexpr uint8_t CODE_NOT_READY = 7;

extern const uint8_t mantisEq[4] PROGMEM;
extern const uint8_t maxMonthDay[] PROGMEM;

class DS3231; // forward


// Represents a time and date, ds3231 year is valid until 2099 (99d), or 2199 century2k=1;
class TimestampDS3231
{
	private:
	btime_t second : 6;
	btime_t minute : 6;
	btime_t hour : 5;
	btime_t weekDay : 3;
	btime_t day : 5;
	btime_t month : 4;
	btime_t am_pm : 1;
	btime_t hourMode : 1;
	btime_t clockHalt : 1;

	uint8_t year : 7;
	uint8_t century2k : 1;

	public:

	void setSecond(uint8_t);
	void setMinute(uint8_t);
	void setHour(uint8_t);
	void setDay(uint8_t);
	void setMonth(uint8_t);
	void setYear(uint8_t);
	void setMD(bool);
	void setHourMode(bool);
	void setCentury(bool);
	void halt(bool);

	
	uint8_t getSecond();
	uint8_t getMinute();
	uint8_t getHour();
	uint8_t getDay();
	uint8_t getMonth();
	uint8_t getYear();
	bool getMD();
	bool getHourMode();
	bool getCentury();
	bool isHalted();
	bool isLeap();

	friend class DS3231;
};

struct rtcFlags
{
	rtcflags_t interrupt : 1;
	rtcflags_t osf : 1;
	rtcflags_t startCode : 3;
	rtcflags_t started : 1;
	rtcflags_t unused : 10;

	rtcFlags();
};

struct DS3231
{
	TimestampDS3231 timestamp;

	rtcFlags flags;
	uint8_t tobcd(uint8_t);
	uint8_t tobin(uint8_t);

	inline void ask(uint8_t _register);
	inline void message(uint8_t _register);
	uint8_t updateTime();
	uint8_t setTime(TimestampDS3231& tsm);
	int8_t itemperature();
	//float temperature();
	uint8_t temperatureMantis();
	uint8_t isAvailable();
	uint8_t getStatus();
	uint8_t getControl();
	uint8_t setStatus(uint8_t f);
	uint8_t setControl(uint8_t f);
	
	void start();

};