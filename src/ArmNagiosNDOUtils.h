/* Asura
   Copyright (C) 2013 MIRACLE LINUX CORPORATION
 
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef ArmNagiosNDOUtils_h
#define ArmNagiosNDOUtils_h

#include <libsoup/soup.h>
#include "ArmBase.h"
#include "ItemTablePtr.h"
#include "JsonParserAgent.h"
#include "JsonBuilderAgent.h"
#include "DBClientConfig.h"
#include "DBClientZabbix.h"

class ArmNagiosNDOUtils : public ArmBase
{
public:
	ArmNagiosNDOUtils(const MonitoringServerInfo &serverInfo);
	virtual ~ArmNagiosNDOUtils();

protected:
	void makeSelectTriggerArg(void);
	void makeSelectEventArg(void);
	void getTrigger(void);
	void getEvent(void);

	// virtual methods
	virtual gpointer mainThread(AsuraThreadArg *arg);
	virtual bool mainThreadOneProc(void);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // ArmNagiosNDOUtils_h
