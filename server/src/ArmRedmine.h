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

#ifndef ArmRedmine_h
#define ArmRedmine_h

#include "ArmBase.h"
#include "DBTablesConfig.h"

class ArmRedmine : public ArmBase
{
public:
	ArmRedmine(const IncidentTrackerInfo &trackerInfo);
	virtual ~ArmRedmine();

protected:
	// virtual methods
	virtual gpointer mainThread(HatoholThreadArg *arg) override;
	virtual ArmBase::ArmPollingResult mainThreadOneProc(void) override;

	std::string getURL(void);
	std::string getQuery(void);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

#endif // ArmRedmine_h
