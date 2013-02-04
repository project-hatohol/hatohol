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

#ifndef ItemGroupTypeManager_h
#define ItemGroupTypeManager_h

#include <glib.h>
#include "ReadWriteLock.h"
#include "ItemGroupType.h"

class ItemGroupTypeManager
{
public:
	static ItemGroupTypeManager *getInstance(void);
	const ItemGroupType *
	getItemGroupType(const ItemDataVector &itemDataVector);

private:
	static ItemGroupTypeManager *m_instance;
	static ReadWriteLock         m_lock;
	static ItemGroupTypeSet      m_groupTypeSet;

	ItemGroupTypeManager(void);
	virtual ~ItemGroupTypeManager();
};

#endif // ItemGroupTypeManager_h


