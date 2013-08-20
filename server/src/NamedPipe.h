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

#ifndef NamedPipe_h
#define NamedPipe_h

#include <string>

class NamedPipe {
public:
	enum EndType {
		END_TYPE_MASTER_READ,
		END_TYPE_MASTER_WRITE,
		END_TYPE_SLAVE_READ,
		END_TYPE_SLAVE_WRITE,
	};

	static const char *BASE_DIR;

	NamedPipe(EndType endType);
	virtual ~NamedPipe();
	bool open(const std::string &name);

protected:
	bool makeBasedirIfNeeded(void);
	bool deleteFileIfExists(const std::string &path);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // NamedPipe_h
