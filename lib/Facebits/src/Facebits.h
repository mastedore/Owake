/* Facebits.h

	Lightweight bitmask manipulation library

	Copyright (c) 2025-2026 Mastedore <marcos@mastedore.com>

	This program is licensed under MIT license. See LICENSE file.
*/

#pragma once

#include <stdint.h>

#define mshift(q) __builtin_ctz((q))
#define mwidth(q) __builtin_popcount((q))

template <unsigned Bits>
constexpr unsigned mask()
{
	static_assert(Bits > 0 && Bits <= sizeof(unsigned) * 8);
	return (1u << Bits) - 1u;
}

template <typename T, T MASK>
struct Facebits_MaskInfo
{
	static constexpr uint8_t shift = mshift(MASK);
	static constexpr uint8_t width = mwidth(MASK);
	static constexpr T maxVal = (T(1) << width) - 1;
};

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