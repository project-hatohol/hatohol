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

#ifndef ActionExecArgMaker_h
#define ActionExecArgMaker_h

#include <memory>
#include "Params.h"
#include "StringUtils.h"

class ActionExecArgMaker
{
private:
	struct Impl;

public:
	ActionExecArgMaker(void);
	virtual ~ActionExecArgMaker();
	void makeExecArg(mlpl::StringVector &argVect, const std::string &cmd);
	static void parseResidentCommand(
	  const std::string &command, std::string &path, std::string &option);

protected:
	static void separatorCallback(const char sep, Impl *impl);

private:
	std::unique_ptr<Impl> m_impl;
};

#endif // ActionExecArgMaker_h

