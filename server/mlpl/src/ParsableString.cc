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

#include "ParsableString.h"
using namespace mlpl;

#include <cstdio>

// ---------------------------------------------------------------------------
// Separator Checker
// ---------------------------------------------------------------------------
SeparatorChecker::SeparatorChecker(const char *separators, bool allowAlter)
: m_savedArrayForAlternative(NULL),
  m_allowAlternative(allowAlter)
{
	m_separatorArray = new bool [ARRAY_SIZE];

	for (int i = 0; i < ARRAY_SIZE; i++)
		m_separatorArray[i] = false;

	for (const char *p = separators; *p; p++) {
		int i = *p;
		m_separatorArray[i] = true;
	}
}

SeparatorChecker::~SeparatorChecker()
{
	if (m_separatorArray)
		delete [] m_separatorArray;
}

bool SeparatorChecker::isSeparator(const char c)
{
	int i = c;
	return m_separatorArray[i];
}

void SeparatorChecker::notifyFound(void)
{
}

void SeparatorChecker::notifyFinish(size_t len)
{
}

void SeparatorChecker::notifyPassedOnSeparator(const char c)
{
}

const bool *SeparatorChecker::getSeparatorArray(void)
{
	return m_separatorArray;
}

void SeparatorChecker::setAlternative(SeparatorChecker *separatorChecker)
{
	if (!m_allowAlternative)
		return;
	if (!m_savedArrayForAlternative)
		m_savedArrayForAlternative = m_separatorArray;
	m_separatorArray = separatorChecker->m_separatorArray;
}

void SeparatorChecker::unsetAlternative(void)
{
	if (!m_savedArrayForAlternative)
		return;
	m_separatorArray = m_savedArrayForAlternative;
	m_savedArrayForAlternative = NULL;
}

void SeparatorChecker::addSeparator(const char *separators)
{
	for (const char *p = separators; *p; p++) {
		int i = *p;
		m_separatorArray[i] = true;
	}
}

// ---------------------------------------------------------------------------
// Separator Checker With Count
// ---------------------------------------------------------------------------
SeparatorCheckerWithCounter::SeparatorCheckerWithCounter(const char *separators)
: SeparatorChecker(separators),
   m_lastSeparator('\0'),
    m_afterFound(false)
{
	m_counter = new int[ARRAY_SIZE];
	resetCounter();
}

SeparatorCheckerWithCounter::~SeparatorCheckerWithCounter()
{
	if (m_counter)
		delete [] m_counter;
}

void SeparatorCheckerWithCounter::resetCounter(void)
{
	for (int i = 0; i < ARRAY_SIZE; i++)
		m_counter[i] = 0;
	m_forwardSeparators.clear();
}

int SeparatorCheckerWithCounter::getCount(const char c)
{
	int i = c;
	return m_counter[i];
}

bool SeparatorCheckerWithCounter::isSeparator(const char c)
{
	bool isSep = SeparatorChecker::isSeparator(c);
	if (isSep) {
		if (m_afterFound) {
			m_forwardSeparators.clear();
			m_afterFound = false;
		}
		separatorFound(c);
		m_lastSeparator = c;
	}
	return isSep;
}

void SeparatorCheckerWithCounter::notifyFound(void)
{
	m_afterFound = true;
}

void SeparatorCheckerWithCounter::notifyFinish(size_t len)
{
	separatorFound('\0');
	m_lastSeparator = '\0';
}

char SeparatorCheckerWithCounter::getLastSeparator(void)
{
	return m_lastSeparator;
}

string &SeparatorCheckerWithCounter::getForwardSeparators(void)
{
	return m_forwardSeparators;
}

void SeparatorCheckerWithCounter::separatorFound(char c)
{
	m_forwardSeparators.push_back(m_lastSeparator);
	int i = m_lastSeparator;
	m_counter[i]++;
}

// ---------------------------------------------------------------------------
// Separator Checker With Callback
// ---------------------------------------------------------------------------
SeparatorCheckerWithCallback::SeparatorCheckerWithCallback(const char *seps)
: SeparatorChecker(seps)
{
	m_callbackArray = new SeparatorCheckerCallbackFunc[ARRAY_SIZE];
	m_argArray = new void *[ARRAY_SIZE];
	for (int i = 0; i < ARRAY_SIZE; i++) {
		m_callbackArray[i] = NULL;
		m_argArray[i] = NULL;
	}
}

SeparatorCheckerWithCallback::~SeparatorCheckerWithCallback()
{
	if (m_callbackArray)
		delete [] m_callbackArray;
	if (m_argArray)
		delete [] m_argArray;
}

void SeparatorCheckerWithCallback::notifyPassedOnSeparator(const char c)
{
	int idx = static_cast<int>(c);
	if (m_callbackArray[idx])
		(*m_callbackArray[idx])(c, m_argArray[idx]);
}

bool
SeparatorCheckerWithCallback::setCallback(const char separator,
                                          SeparatorCheckerCallbackFunc func,
                                          void *arg)
{
	int idx = static_cast<int>(separator);
	const bool *separatorArray = getSeparatorArray();
	if (!separatorArray[idx])
		return false;
	m_callbackArray[idx] = func;
	m_argArray[idx] = arg;
	return true;
}

// ---------------------------------------------------------------------------
// Publice Methods
// ---------------------------------------------------------------------------
SeparatorChecker ParsableString::SEPARATOR_SPACE(" ", false);
SeparatorChecker ParsableString::SEPARATOR_COMMA(",", false);
SeparatorChecker ParsableString::SEPARATOR_QUOT("'", false);
SeparatorChecker ParsableString::SEPARATOR_PARENTHESIS("()", false);

ParsableString::ParsableString(const ParsableString &parsable)
: m_string(parsable.m_string),
  m_parsingPosition(m_string.c_str()),
  m_pendingNotifySeparator('\0')
{
}

ParsableString::ParsableString(const string &str)
: m_string(str),
  m_parsingPosition(m_string.c_str()),
  m_pendingNotifySeparator('\0')
{
}

ParsableString::~ParsableString()
{
	m_parsingPosition = m_string.c_str();
}

bool ParsableString::finished(void)
{
	return (m_parsingPosition == NULL);
}

string ParsableString::readWord(SeparatorChecker &checker,
                                bool keepParsingPosition)
{
	if (m_parsingPosition == NULL)
		return "";
	m_keepParsingPosition = keepParsingPosition;
	m_readWord.clear();
	genericParse(m_parsingPosition, checker,
	             &ParsableString::readWordAction, NULL,
	             &ParsableString::readWordFinish, NULL);
	return m_readWord.c_str();
}

string ParsableString::readWord(const char *separators,
                                bool keepParsingPosition)
{
	SeparatorChecker checker(separators);
	return readWord(checker, keepParsingPosition);
}

const char *ParsableString::getString(void) const
{
	return m_string.c_str();
}

ParsingPosition ParsableString::getParsingPosition(void) const
{
	return m_parsingPosition;
}

void ParsableString::setParsingPosition(ParsingPosition position)
{
	m_parsingPosition = position;
}

// ---------------------------------------------------------------------------
// Protected Methods
// ---------------------------------------------------------------------------
void ParsableString::genericParse(const char *target,
                                  SeparatorChecker &checker,
                                  WordFoundActionFunc foundFunc,
                                  void *foundArg,
                                  ParseFinishActionFunc finishFunc,
                                  void *finishArg)
{
	if (m_pendingNotifySeparator) {
		checker.notifyPassedOnSeparator(m_pendingNotifySeparator);
		m_pendingNotifySeparator = '\0';
	}

	size_t len = 0;
	const char *currWordHead = target;
	const char *ptr = target;
	for (; *ptr; ptr++) {
		if (!checker.isSeparator(*ptr)) {
			len++;
			continue;
		}

		if (len > 0) {
			checker.notifyFound();
			bool exitFlag = (this->*foundFunc)(currWordHead,
			                                   len, ptr, foundArg);
			if (exitFlag) {
				m_pendingNotifySeparator = *ptr;
				return;
			}
			len = 0;
		} 
		checker.notifyPassedOnSeparator(*ptr);
		currWordHead = ptr + 1;
	}
	if (len > 0) {
		checker.notifyFound();
		(this->*foundFunc)(currWordHead, len, ptr, foundArg);
	}

	if (finishFunc)
		(this->*finishFunc)(finishArg);
	checker.notifyFinish(len);
}

bool ParsableString::readWordAction(const char *wordHead, size_t length,
                                    const char *currPtr, void *arg)
{
	m_readWord = string(wordHead, length);
	if (!m_keepParsingPosition)
		m_parsingPosition = (currPtr + 1);
	return true;
}

void ParsableString::readWordFinish(void *arg)
{
	if (!m_keepParsingPosition)
		m_parsingPosition = NULL;
}
