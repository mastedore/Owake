/*
	rtc3231.cpp

	A simple lightweight driver library for the DS3231 RTC 

	Copyright (C) 2026 Marcos Rubiano
	email:	markusianito@proton.me

	This program is licensed under MIT license. See LICENSE file.
	
*/


// fuck off Wconversion shit, i know what the fuck im doing.
#pragma GCC diagnostic ignored "-Wconversion"

#include <Arduino.h>
#include <Wire.h>
#include "rtc3231.h"

#define sendTime(x) Wire.write(tobcd(x))


constexpr uint8_t REG0_CLOCKHALT_MASK = 	0b10000000;
constexpr uint8_t REG0_SECONDS_MASK = 		0b01111111;
constexpr uint8_t REG1_MINUTES_MASK = 		0b01111111;
constexpr uint8_t REG2_HOURMODE_MASK = 		0b01000000;
constexpr uint8_t REG2_AMPM_MASK = 			0b00100000;
constexpr uint8_t REG2_HOUR_MASK = 			0b00011111;
constexpr uint8_t REG2_HOURS_MASK = 		0b00111111;
constexpr uint8_t REG3_WEEKDAY_MASK = 		0b00000111;
constexpr uint8_t REG4_DAYS_MASK = 			0b00111111;
constexpr uint8_t REG5_CENTURY_MASK = 		0b10000000;
constexpr uint8_t REG5_MONTH_MASK = 		0b00011111;
constexpr uint8_t REG12_MSBTEMP_MASK = 		0b11000000;

const uint8_t mantisEq[4] PROGMEM = {0, 25, 50, 75};
const uint8_t maxMonthDay[] PROGMEM = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

rtcFlags::rtcFlags() :
	interrupt(0),
	osf(0),
	startCode(CODE_NOT_READY),
	started(0)
{
}




void TimestampDS3231::setSecond(uint8_t x){	second = x%60;	}
void TimestampDS3231::setMinute(uint8_t x){	minute = x%60;	}
void TimestampDS3231::setHour(uint8_t x)
{
	if (hourMode == HMODE_24)
	{
		hour = x%24;
	}
	else
	{
		hour = constrain(x,1,12);
	}
}
void TimestampDS3231::setDay(uint8_t x)
{
	if ((month == 2) && isLeap())
	{
		day = constrain(x, 1, 29);
	}
	else
	{
		day = constrain(x, 1, maxMonthDay[month-1]);
	}
}
void TimestampDS3231::setMonth(uint8_t x){		month = constrain(x, 1, 12);	}
void TimestampDS3231::setYear(uint8_t x){		year= x%100;}
void TimestampDS3231::setMD(bool x){			am_pm = x;	}
void TimestampDS3231::setHourMode(bool x){	hourMode = x;	}
void TimestampDS3231::setCentury(bool x){	century2k = x;	}
void TimestampDS3231::halt(bool x){			clockHalt = x;	}


uint8_t TimestampDS3231::getSecond(){	return second;			}
uint8_t TimestampDS3231::getMinute(){	return minute;			}
uint8_t TimestampDS3231::getHour(){		return hour;			}
uint8_t TimestampDS3231::getDay(){		return day;				}
uint8_t TimestampDS3231::getMonth(){	return month;			}
uint8_t TimestampDS3231::getYear(){		return year;			}
bool TimestampDS3231::getMD(){			return am_pm;			}
bool TimestampDS3231::getHourMode(){	return hourMode;		}
bool TimestampDS3231::getCentury(){		return century2k;		}
bool TimestampDS3231::isHalted(){		return clockHalt;		}
bool TimestampDS3231::isLeap(){			return (year % 4) == 0;	}



inline void DS3231::ask(uint8_t reg)
{
	message(reg);
	Wire.endTransmission(false);
}
inline void DS3231::message(uint8_t reg)
{
	Wire.beginTransmission(RTC_I2C_ADDRESS);
	Wire.write(reg);
}


uint8_t DS3231::tobcd(uint8_t x)
{
	return ((x / 10) << 4) | (x % 10);
}
uint8_t DS3231::tobin(uint8_t x)
{
	return (x >> 4) * 10 + (x & 0x0f);
}


uint8_t DS3231::setTime(TimestampDS3231& tsm)
{
	if (!flags.started) {return CODE_NOT_READY;}

	uint8_t code = isAvailable();
	if (code != 0) {return code;}

	message(RTC_REGISTER_TIME);

	uint8_t s = tobcd(tsm.second) | (tsm.clockHalt ? REG0_CLOCKHALT_MASK : 0);
	Wire.write(s);


	Wire.write(tobcd(tsm.minute % 60));


	if (tsm.hourMode == HMODE_12)
	{
		uint8_t hr = tobcd(
			(tsm.hour>=1 && tsm.hour<=12) ?
			tsm.hour :
			static_cast<btime_t>((tsm.hour%13) + 1)
		);

		Wire.write(REG2_HOURMODE_MASK | hr | (tsm.am_pm ? REG2_AMPM_MASK : 0));
	} else
	{
		Wire.write(tobcd(tsm.hour%24) & REG2_HOURS_MASK);
	}


	Wire.write(tobcd(tsm.weekDay) & REG3_WEEKDAY_MASK);


	Wire.write(tobcd(tsm.day) & REG4_DAYS_MASK);

	uint8_t mth = tobcd(
		(tsm.month>=1 && tsm.month<=12) ?
		tsm.month :
		static_cast<btime_t>((tsm.month%13) + 1)
	);
	Wire.write(mth | (tsm.century2k ? REG5_CENTURY_MASK : 0));


	Wire.write(tobcd(tsm.year));

	return Wire.endTransmission();
}

uint8_t DS3231::updateTime()
{
	uint8_t code = isAvailable();
	if (code != 0) {return code;}
	
	ask(RTC_REGISTER_TIME);

	Wire.requestFrom(RTC_I2C_ADDRESS, uint8_t(7));

	uint8_t s = Wire.read();
	timestamp.clockHalt = s & REG0_CLOCKHALT_MASK;
	timestamp.second = tobin(s & REG0_SECONDS_MASK);
	timestamp.minute = tobin(Wire.read() & REG1_MINUTES_MASK);
	

	uint8_t h = Wire.read();
	uint8_t mm = (h & REG2_AMPM_MASK);
	timestamp.hourMode = h & REG2_HOURMODE_MASK;
	timestamp.am_pm = mm;
	timestamp.hour = tobin(mm ? (h & REG2_HOUR_MASK) : (h & (REG2_HOUR_MASK | REG2_AMPM_MASK)));


	timestamp.weekDay = Wire.read() & REG3_WEEKDAY_MASK;

	timestamp.day = tobin(Wire.read() & REG4_DAYS_MASK);

	
	uint8_t M = Wire.read();
	timestamp.century2k = M & REG5_CENTURY_MASK;
	timestamp.month = tobin(M & REG5_MONTH_MASK);


	timestamp.year = tobin(Wire.read());

	return 0;
}

uint8_t DS3231::temperatureMantis()
{
	uint8_t code = isAvailable();
	if (code != 0) {return code;}
	ask(RTC_REGISTER_TEMP+1);
	Wire.requestFrom(RTC_I2C_ADDRESS, uint8_t(1));

	return mantisEq[(Wire.read() & REG12_MSBTEMP_MASK)];
}
int8_t DS3231::itemperature()
{
	uint8_t code = isAvailable();
	if (code != 0) {return 0;}

	ask(RTC_REGISTER_TEMP);

	Wire.requestFrom(RTC_I2C_ADDRESS, uint8_t(1));

	return Wire.read();
}

uint8_t DS3231::isAvailable()
{
	Wire.beginTransmission(RTC_I2C_ADDRESS);
	return Wire.endTransmission();
}

uint8_t DS3231::getControl()
{
	ask(RTC_REGISTER_CONTROL);

	Wire.requestFrom(RTC_I2C_ADDRESS, uint8_t(1));
	return Wire.read();
}
uint8_t DS3231::getStatus()
{
	ask(RTC_REGISTER_STATUS);

	Wire.requestFrom(RTC_I2C_ADDRESS, uint8_t(1));
	return Wire.read();
}

uint8_t DS3231::setStatus(uint8_t f)
{
	message(RTC_REGISTER_STATUS);
	Wire.write(f);

	return Wire.endTransmission();
}
uint8_t DS3231::setControl(uint8_t f)
{
	message(RTC_REGISTER_CONTROL);
	Wire.write(f);

	return Wire.endTransmission();
}



void DS3231::start()
{
	Wire.begin();
	uint8_t code = isAvailable();

	flags.startCode = code & 0x7;
	if (code == 0)
	{
		flags.osf = getStatus() & 0x80;
		flags.started = true;
	}
}