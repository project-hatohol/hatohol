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

#include "ActionExecArgMaker.h"
#include "ParsableString.h"
using namespace mlpl;

struct ActionExecArgMaker::PrivateContext {

	// member for a command line parsing
	SeparatorCheckerWithCallback separator;
	bool inQuot;
	bool byBackSlash;
	string currWord;
	StringVector *argVect;

	PrivateContext(void)
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
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();
	m_ctx->separator.setCallbackTempl<PrivateContext>
	  (' ', separatorCallback, m_ctx);
	m_ctx->separator.setCallbackTempl<PrivateContext>
	  ('\'', separatorCallback, m_ctx);
	m_ctx->separator.setCallbackTempl<PrivateContext>
	  ('\\', separatorCallback, m_ctx);
}

ActionExecArgMaker::~ActionExecArgMaker()
{
	if (m_ctx)
		delete m_ctx;
}

void ActionExecArgMaker::makeExecArg(StringVector &argVect, const string &cmd)
{
	m_ctx->resetParser(&argVect);
	ParsableString parsable(cmd);
	while (!parsable.finished()) {
		string word = parsable.readWord(m_ctx->separator);
		m_ctx->currWord += word;
		m_ctx->byBackSlash = false;
	}
	m_ctx->pushbackCurrWord();
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
void ActionExecArgMaker::separatorCallback(const char sep, PrivateContext *ctx)
{
	if (sep == ' ') {
		if (ctx->inQuot)
			ctx->currWord += ' ';
		else
			ctx->pushbackCurrWord();
		ctx->byBackSlash = false;
	} else if (sep == '\'') {
		if (ctx->byBackSlash)
			ctx->currWord += "'";
		else
			ctx->inQuot = !ctx->inQuot;
		ctx->byBackSlash = false;
	} else if (sep == '\\') {
		if (ctx->byBackSlash)
			ctx->currWord += '\\';
		ctx->byBackSlash = !ctx->byBackSlash;
	}
}

