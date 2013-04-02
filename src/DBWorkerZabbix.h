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

#ifndef DBWorkerZabbix_h
#define DBWorkerZabbix_h

#include "DBAgent.h"

class DBWorkerZabbix {
public:
	DBWorkerZabbix(void);
	virtual ~DBWorkerZabbix();

protected:
	void createTableTriggersRaw2_0(void);

private:
	DBAgent *m_dbAgent;
};

#endif // DBWorkerZabbix_h
