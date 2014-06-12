/*
 * Copyright (C) 2014 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Hatohol. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef EndianConverter_h
#define EndianConverter_h

#if defined __x86_64__ || defined __i386__
class EndianConverter {
public:
	/**
	 * Convert native endian to little endian.
	 */
	template<typename T>
	static inline T NtoL(const T &val)
	{
		return val;
	}

	static inline uint16_t NtoL(const uint16_t &val)
	{
		return val;
	}

	static inline uint32_t NtoL(const uint32_t &val)
	{
		return val;
	}

	static inline uint64_t NtoL(const uint64_t &val)
	{
		return val;
	}

	/**
	 * Convert little endian to native endian.
	 */
	template<typename T>
	static inline T LtoN(const T &val)
	{
		return val;
	}

	static inline uint16_t LtoN(const uint16_t &val)
	{
		return val;
	}

	static inline uint32_t LtoN(const uint32_t &val)
	{
		return val;
	}

	static inline uint64_t LtoN(const uint64_t &val)
	{
		return val;
	}
};
#endif // defined __x86_64__ || defined __i386__

#endif // EndianConverter_h
