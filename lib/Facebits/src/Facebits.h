/* Facebits.h

	Lightweight bitmask manipulation library

	Copyright (c) 2025-2026 Mastedore <marcos@mastedore.com>

	This program is licensed under MIT license. See LICENSE file.
*/

#pragma once

#include <stdint.h>

// The "ll" builtins, because plain int is 16 bits on AVR and the int versions
// would cut off masks that live above bit 15 of a uint32_t.
#define mshift(q) __builtin_ctzll((q))
#define mwidth(q) __builtin_popcountll((q))

// mask<3>() == 0b111. Handy for clipping a value before it goes into a bitfield.
template <unsigned Bits>
constexpr unsigned mask()
{
	static_assert(Bits > 0 && Bits <= sizeof(unsigned) * 8);
	// Shifting by the full width of the type is undefined, so that case is spelled out.
	if constexpr (Bits == sizeof(unsigned) * 8)
	{
		return ~0u;
	}
	else
	{
		return (1u << Bits) - 1u;
	}
}

template <typename T, T MASK>
struct Facebits_MaskInfo
{
	static constexpr uint8_t shift = mshift(MASK);
	static constexpr uint8_t width = mwidth(MASK);
	static constexpr T maxVal = (T(1) << width) - 1;
};

// MASK says where the field lives inside info. For example
// writeData<uint8_t, 0b00110000>(x, 2) stores 2 in bits 4-5 and leaves the
// rest of x alone, and readData<uint8_t, 0b00110000>(x) gives the 2 back.
template <typename T, T MASK>
static inline T readData(T info) { return (info & MASK) >> mshift(MASK); }

template <typename T, T MASK, typename U>
static inline void writeData(T &info, U data)
{
	info = (info & ~MASK) | ((static_cast<T>(data) & Facebits_MaskInfo<T, MASK>::maxVal) << Facebits_MaskInfo<T, MASK>::shift);
}

template <typename T, T MASK, typename U>
static inline T writeDataPreview(T info, U data)
{
	writeData<T, MASK>(info, data);
	return info;
}

template <typename T, T MASK, typename U>
static inline T writeDataAndGet(T &info, U data)
{
	writeData<T, MASK>(info, data);
	return info;
}