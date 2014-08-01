/*
 * Copyright (C) 2013-2014 Project Hatohol
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

#include <cstdlib>
#include "HatoholException.h"
#include "Utils.h"
using namespace std;
using namespace mlpl;

bool HatoholException::m_saveStackTrace = false;
const int HatoholException::UNKNOWN_LINE_NUMBER = -1;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void HatoholException::init(void)
{
	char *env = getenv(HATOHOL_STACK_TRACE_SET_ENV);
	if (!env)
		return;
	if (string("1") == env)
		m_saveStackTrace = true;
}

HatoholException::HatoholException(
  const string &brief, const string &sourceFileName, const int &lineNumber)
: m_what(brief),
  m_sourceFileName(sourceFileName),
  m_lineNumber(lineNumber)
{
	if (m_saveStackTrace)
		saveStackTrace();
}

HatoholException::~HatoholException() _HATOHOL_NOEXCEPT
{
}

const char* HatoholException::what() const _HATOHOL_NOEXCEPT
{
	// The 'char' pointer this function returns may be used after this
	// function returns in the caller. If we return
	// 'getFancyMessage().c_str()', the pointed region will be
	// destroyed at the end of this function. As a result, the caller
	// has an invalid pointer.
	// Actually, we got an empty string on the defualt exception handler
	// of the system on Ubuntu 13.04.
	// So we here return 'char' pointer of a string member variable,
	// which is valid until this object is destroyed.
	m_whatCache = getFancyMessage();
	return m_whatCache.c_str();
}

string HatoholException::getFancyMessage(void) const
{
	string msg =
	   StringUtils::sprintf("<%s:%d> %s\n",
	                        getSourceFileName().c_str(),
	                        getLineNumber(), m_what.c_str());
	if (m_saveStackTrace)
		msg += m_stackTrace;
	return msg;
}

const string &HatoholException::getSourceFileName(void) const
{
	return m_sourceFileName;
}

int HatoholException::getLineNumber(void) const
{
	return m_lineNumber;
}

const string &HatoholException::getStackTrace(void) const
{
	return m_stackTrace;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void HatoholException::saveStackTrace(void)
{
	void *trace[128];
	int n = backtrace(trace, sizeof(trace) / sizeof(trace[0]));
	m_stackTrace = Utils::makeDemangledStackTraceLines(trace, n);
}

void HatoholException::setBrief(const string &brief)
{
	m_what = brief;
}

// ---------------------------------------------------------------------------
// ExceptionCatcher
// ---------------------------------------------------------------------------
bool ExceptionCatchable::exec(void)
{
	bool caughtException = false;
	try {
		(*this)();
	} catch (const HatoholException &e) {
		onCaught(e);
		caughtException = true;
	} catch (const exception &e) {
		onCaught(e);
		caughtException = true;
	} catch (...) {
		onCaught();
		caughtException = true;
	}
	return caughtException;
}

void ExceptionCatchable::onCaught(const HatoholException &e)
{
	MLPL_ERR("Got Hatohol exception: %s\n",
	         e.getFancyMessage().c_str());
}

void ExceptionCatchable::onCaught(const exception &e)
{
	MLPL_ERR("Got exception: %s\n", e.what());
}

void ExceptionCatchable::onCaught(void)
{
	MLPL_ERR("Got unknown exception\n");
}
