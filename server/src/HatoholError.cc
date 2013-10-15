/*
 * Copyright (C) 2013 Project Hatohol
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

#include "HatoholError.h"
using namespace std;
static const char *errorMessages[NUM_HATOHOL_ERROR_CODE];

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void HatoholError::init(void)
{
	errorMessages[HTERR_OK] = "OK.";
	errorMessages[HTERR_UNINITIALIZED] =
	  "Uninitialized (This is probably a bug).";
}

HatoholError::HatoholError(const HatoholErrorCode &code,
                           const string &optMessage)
: m_code(code)
{
	if (!optMessage.empty())
		m_optMessage = optMessage;
}

HatoholError::~HatoholError(void)
{
}

const HatoholErrorCode &HatoholError::getErrorCode(void) const
{
	return m_code;
}

const string &HatoholError::getOptMessage(void) const
{
	return m_optMessage;
}

bool HatoholError::operator==(const HatoholErrorCode &rhs) const
{
	return m_code == rhs;
}

bool HatoholError::operator!=(const HatoholErrorCode &rhs) const
{
	return m_code != rhs;
}
