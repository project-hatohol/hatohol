/*
 * Copyright (C) 2013 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, version 3
 * as published by the Free Software Foundation.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Hatohol. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#pragma once
#include <vector>
#include <list>
#include <string>
#include <queue>

namespace mlpl {

// ---------------------------------------------------------------------------
// class: SeparatorChecker
// ---------------------------------------------------------------------------
class SeparatorChecker {
public:
	SeparatorChecker(const char *separators, bool allowAlter = true);
	virtual ~SeparatorChecker();
	virtual bool isSeparator(const char c);
	virtual void notifyFound(void);
	virtual void notifyFinish(size_t len);
	virtual void notifyPassedOnSeparator(const char c);
	virtual void setAlternative(SeparatorChecker *separatorChecker);
	virtual void unsetAlternative();
	virtual void addSeparator(const char *separators);

protected:
	static const int ARRAY_SIZE = 0x100;
	const bool *getSeparatorArray(void);

private:
	bool *m_separatorArray;
	bool *m_savedArrayForAlternative;
	bool  m_allowAlternative;
};

// ---------------------------------------------------------------------------
// class: SeparatorCheckerWithCounter
// ---------------------------------------------------------------------------
class SeparatorCheckerWithCounter : public SeparatorChecker {
public:
	SeparatorCheckerWithCounter(const char *separators);
	void resetCounter(void);
	int getCount(const char c);
	char getLastSeparator(void);
	std::string &getForwardSeparators(void);
	virtual ~SeparatorCheckerWithCounter();
	virtual bool isSeparator(const char c);
	virtual void notifyFound(void);
	virtual void notifyFinish(size_t len);

protected:
	void separatorFound(char c);

private:
	int *m_counter;
	char m_lastSeparator;
	bool m_afterFound;
	std::string m_forwardSeparators;
};

// ---------------------------------------------------------------------------
// class: SeparatorCheckerWithCallback
// ---------------------------------------------------------------------------
typedef void (*SeparatorCheckerCallbackFunc)(const char separator, void *arg);

class SeparatorCheckerWithCallback : public SeparatorChecker {
public:
	SeparatorCheckerWithCallback(const char *separators);
	virtual ~SeparatorCheckerWithCallback();

	virtual void notifyPassedOnSeparator(const char c);

	bool setCallback(const char separator,
	                 SeparatorCheckerCallbackFunc func, void *arg);
	template <typename T>
	bool setCallbackTempl(const char separator,
	                 void (* func)(const char sep, T *arg), T *arg)
	{
		bool ret =
		  setCallback(separator,
		    reinterpret_cast<SeparatorCheckerCallbackFunc>(func), arg);
		return ret;
	}

private:
	SeparatorCheckerCallbackFunc *m_callbackArray;
	void                        **m_argArray;
};

// ---------------------------------------------------------------------------
// class: ParsableString
// ---------------------------------------------------------------------------
typedef const char * ParsingPosition;

class ParsableString {
public:
	static SeparatorChecker SEPARATOR_SPACE;
	static SeparatorChecker SEPARATOR_COMMA;
	static SeparatorChecker SEPARATOR_QUOT;
	static SeparatorChecker SEPARATOR_PARENTHESIS;

	ParsableString(const ParsableString &parsable);
	ParsableString(const std::string &str);
	virtual ~ParsableString();

	bool finished(void);
	std::string readWord(const char *separators,
	                     bool keepParsingPosition = false);
	std::string readWord(SeparatorChecker &checker,
	                     bool keepParsingPosition = false);
	const char *getString(void) const;
	ParsingPosition getParsingPosition(void) const;
	void setParsingPosition(ParsingPosition position);

protected:
	/*
	 * When WorFoundActionFunc returns true,
	 * genericParse() ends immediately.
	 */
	typedef bool
	(ParsableString::*WordFoundActionFunc)(const char *wordHead, size_t len,
	                                       const char *currPtr, void *arg);
	typedef void
	(ParsableString::*ParseFinishActionFunc)(void *arg);

	void genericParse(const char *target, SeparatorChecker &checker,
	                  WordFoundActionFunc foundFunc,
	                  void *foundArg = NULL,
	                  ParseFinishActionFunc finishFunc = NULL,
	                  void *finishArg = NULL);
	bool readWordAction(const char *wordHead, size_t len,
	                    const char *currPtr, void *arg);
	void readWordFinish(void *arg);

private:
	std::string m_string;
	std::string m_readWord;
	const char *m_parsingPosition;
	bool        m_keepParsingPosition;
	char        m_pendingNotifySeparator;
};

} // namespace mlpl

