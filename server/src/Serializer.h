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

#ifndef Serializer_h
#define Serializer_h

#if defined __x86_64__ || defined __i386__
class Serializer {
	static inline void writeLE(uint8_t &dest, const uint8_t &src)
	{
		dest = src;
	}

	static inline void writeLE(uint16_t &dest, const uint16_t &src)
	{
		dest = src;
	}

	static inline void writeLE(uint32_t &dest, const uint32_t &src)
	{
		dest = src;
	}

	static inline void writeLE(uint64_t &dest, const uint64_t &src)
	{
		dest = src;
	}
};
#endif // defined __x86_64__ || defined __i386__

#endif // Serializer_h
