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

#ifndef FaceRest_h
#define FaceRest_h

#include <libsoup/soup.h>
#include "FaceBase.h"

class FaceRest : public FaceBase {
public:
	FaceRest(CommandLineArg &cmdArg);
	virtual ~FaceRest();

protected:
	// virtual methods
	gpointer mainThread(AsuraThreadArg *arg);

	// generic sub routines
	size_t parseCmdArgPort(CommandLineArg &cmdArg, size_t idx);

private:
	guint       m_port;
	SoupServer *m_soupServer;
};

#endif // FaceRest_h
