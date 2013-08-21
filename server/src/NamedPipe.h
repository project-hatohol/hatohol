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
#include <glib.h>
#include <SmartBuffer.h>

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
	bool openPipe(const std::string &name);
	bool createGIOChannel(GIOCondition cond, GIOFunc iochCb = NULL,
	                      gpointer data = NULL);
	int getFd(void) const;
	const string &getPath(void) const;

	/**
	 * Push the buffer to the pipe. After the call, the content of
	 * the buffer is taken over (i.e. the buffer in 'buf' is moved).
	 * The function is asynchronus and returns without blocking.
	 */
	void push(mlpl::SmartBuffer &buf);

protected:
	static gboolean writeCb(GIOChannel *source, GIOCondition condition,
	                        gpointer data);
	/**
	 * Write data to the pipe.
	 *
	 * @parameter buf
	 * A SmartBuffer instance that has data to be written.
	 *
	 * @parameter fullyWritten
	 * When the all data in \buf was written, this function set
	 * true to the parameter. Otherwise it sets false.
 	 *
	 * @return
	 * If no error occurs, true is returned. Otherwise false.
	 */
	bool writeBuf(SmartBuffer &buf, bool &fullyWritten);

	void enableWriteCbIfNeeded(void);
	bool isExistingDir(const string &dirname, bool &hasError);
	bool makeBasedirIfNeeded(const string &baseDir);
	bool deleteFileIfExists(const std::string &path);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // NamedPipe_h
