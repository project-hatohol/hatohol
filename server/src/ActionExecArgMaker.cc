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

#include "ActionExecArgMaker.h"
#include "ParsableString.h"
using namespace std;
using namespace mlpl;

struct ActionExecArgMaker::Impl {

	// member for a command line parsing
	SeparatorCheckerWithCallback separator;
	bool inQuot;
	bool byBackSlash;
	string currWord;
	StringVector *argVect;

	Impl(void)
	: separator(" '\\"),
	  inQuot(false),
	  byBackSlash(false),
	  argVect(NULL)
	{
	}

	void resetParser(StringVector *_argVect)
	{
		inQuot = false;
		byBackSlash = false;
		currWord.clear();
		argVect = _argVect;
	}

	void pushbackCurrWord(void)
	{
		if (currWord.empty())
			return;
		argVect->push_back(currWord);
		currWord.clear();
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ActionExecArgMaker::ActionExecArgMaker()
: m_impl(new Impl())
{
	m_impl->separator.setCallbackTempl<Impl>
	  (' ', separatorCallback, m_impl.get());
	m_impl->separator.setCallbackTempl<Impl>
	  ('\'', separatorCallback, m_impl.get());
	m_impl->separator.setCallbackTempl<Impl>
	  ('\\', separatorCallback, m_impl.get());
}

ActionExecArgMaker::~ActionExecArgMaker()
{
}

void ActionExecArgMaker::makeExecArg(StringVector &argVect, const string &cmd)
{
	m_impl->resetParser(&argVect);
	ParsableString parsable(cmd);
	while (!parsable.finished()) {
		string word = parsable.readWord(m_impl->separator);
		m_impl->currWord += word;
		m_impl->byBackSlash = false;
	}
	m_impl->pushbackCurrWord();
}

void ActionExecArgMaker::parseResidentCommand(
  const string &command, string &path, string &option)
{
	size_t posSpace = command.find(' ');
	if (posSpace == string::npos) {
		path = command;
		option.clear();
		return;
	}
	
	path = string(command, 0, posSpace);
	string optionRaw = string(command, posSpace);
	option = StringUtils::stripBothEndsSpaces(optionRaw);
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void ActionExecArgMaker::separatorCallback(const char sep, Impl *impl)
{
	if (sep == ' ') {
		if (impl->inQuot)
			impl->currWord += ' ';
		else
			impl->pushbackCurrWord();
		impl->byBackSlash = false;
	} else if (sep == '\'') {
		if (impl->byBackSlash)
			impl->currWord += "'";
		else
			impl->inQuot = !impl->inQuot;
		impl->byBackSlash = false;
	} else if (sep == '\\') {
		if (impl->byBackSlash)
			impl->currWord += '\\';
		impl->byBackSlash = !impl->byBackSlash;
	}
}

