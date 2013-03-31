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

#ifndef VirtualDataStore_h
#define VirtualDataStore_h

#include <stdint.h>

#include "VirtualDataStore.h"
#include "DataStoreManager.h"

enum TriggerStatusType {
	TRIGER_STATUS_OK,
	TRIGER_STATUS_PROBLEM,
};

enum TriggerSeverityType {
	TRIGGER_SEVERITY_INFO,
	TRIGGER_SEVERITY_WARN,
};

struct TriggerInfo {
	TriggerStatusType   status;
	TriggerSeverityType severity;
	timespec            lastChangedTime;
	uint32_t            serverId;
	string              hostId;
	string              hostName;
	string              brief;
};

typedef list<TriggerInfo>         TriggerInfoList;
typedef TriggerInfoList::iterator TriggerInfoListIterator;

class VirtualDataStore : public DataStoreManager
{
public:
	VirtualDataStore(void);
	virtual ~VirtualDataStore(void);

	virtual void getTriggerList(TriggerInfoList &triggerList) = 0;
};

#endif // VirtualDataStore_h
