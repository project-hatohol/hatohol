/*
 * Copyright (C) 2013 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, version 3
 * as published by the Free Software Foundation.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Hatohol. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#pragma once
#include <string>
#include <memory>
#include <glib.h>
#include "Params.h"
#include "SmartBuffer.h"

class NamedPipe {
public:
	enum EndType {
		END_TYPE_MASTER_READ,
		END_TYPE_MASTER_WRITE,
		END_TYPE_SLAVE_READ,
		END_TYPE_SLAVE_WRITE,
	};

	/**
	 * The callback function type used for pull().
	 *
	 * @parameter stat
	 * When data is successfully obtained, stat is G_IO_STATUS_NORMAL.
	 * When any problem occurs, other value is set.
	 *
	 * @parameter buf
	 * A SmartBuffer instance that has the obtained data. If you need
	 * to use the data after the callback function returns, you have to
	 * get buffer by calling buf->takeOver() or copy it with any way such
	 * as memcpy(). The index points the top of it (i.e. 0).
	 *
	 * @parameter size
	 * The size of the obtained data. This is the same as the requested
	 * when read is succeeded (stat is G_IO_STATUS_NORMAL).
	 * Note that the size of buf may be larger than the requested.
	 *
	 * @parameter priv
	 * A pointer passed as 'priv' in the call of pull().
	 */
	typedef void (*PullCallback)(GIOStatus stat, mlpl::SmartBuffer &buf,
	                             size_t size, void *priv);

	typedef void (*TimeoutCallback)(NamedPipe *namedPipe, void *priv);

	static const char *BASE_DIR;

	NamedPipe(EndType endType);
	virtual ~NamedPipe();

	/**
	 * Open the pipe and register an event callback.
	 * openPipe() must be called before the call of this function.
	 *
	 * @parameter iochCb
	 * A callback handler on the pipe's event.
	 * If the instance type is END_TYPE_MASTER_READ or 
	 * END_TYPE_SLAVE_READ, it is called on one of the events:
	 * G_IO_PRI, G_IO_ERR, G_IO_HUP, and G_IO_NVAL.
	 * If the instance type is END_TYPE_MASTER_WRITE or 
	 * END_TYPE_SLAVE_WRITE, it is call on one of the events:
	 * G_IO_ERR, G_IO_HUP, and G_IO_NVAL.
	 *
	 * @parameter data
	 * An arbitray user data that is passed as an argument of the callback.
	 *
	 * @param glibMainContext
	 * A GLibMainContext for the event callback. If this paramters is
	 * omitted, the default context is used. g_main_contet_ref() is
	 * called for this object.
	 *
	 * @return
	 * If no error occurs, true is returned. Otherwise false.
	 */
	bool init(const std::string &name, GIOFunc iochCb, gpointer data,
	          GMainContext *glibMainContext = NULL);

	int getFd(void) const;
	const std::string &getPath(void) const;

	/**
	 * Push the buffer to the internal queue. After the call,
	 * the content of the buffer is taken over (i.e. the buffer in
	 * 'buf' is moved).
	 * The function is asynchronous and returns without blocking.
	 *
	 * @param buf A SmartBuffer instance that has data to be written.
	 */
	void push(mlpl::SmartBuffer &buf);

	/**
	 * Read data asynchronously from the pipe. This function cannot be
	 * called before the previous call is completed. (i.e. the callback
	 * of the previous call is happened)
	 *
	 * @param size
	 * A request size to be read.
	 *
	 * @param callback
	 * A function that will be called back when data with the requested
	 * size is ready.
	 *
	 * @param priv
	 * A pointer passed to the callback function.
	 *
	 */
	void pull(size_t size, PullCallback callback, void *priv);

	/**
	 * Set a pull or push time-out callback function. If a pull or push
	 * operation is not completed within the time-out value, the specified
	 * function is called back.
	 * If the time-out value and the callback function are already set,
	 * they are cancelled and will be activated at
	 * the next pull(), push(), or an internal start of sending next buffer.
	 *
	 * @param timeout
	 * A timeout value in millisecond. If this parameter is 0, the current
         * time-out callback is cancelled.
	 *
	 * @param timeoutCb
	 * A callback function.
	 *
	 * @param priv
	 * A pointer passed to the callback function.
	 *
	 */
	void setTimeout(unsigned int timeout,
	                TimeoutCallback timeoutCb, void *priv);

protected:
	static gboolean writeCb(GIOChannel *source, GIOCondition condition,
	                        gpointer data);
	static gboolean writeErrorCb(GIOChannel *source, GIOCondition condition,
	                             gpointer data);
	static gboolean readCb(GIOChannel *source, GIOCondition condition,
	                        gpointer data);
	static gboolean readErrorCb(GIOChannel *source, GIOCondition condition,
	                            gpointer data);

	bool openPipe(const std::string &name);

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
	 * @parameter flush
	 * When this parameter is true, the written data is flushed.
	 *
	 * @return
	 * If no error occurs, true is returned. Otherwise false.
	 */
	bool writeBuf(mlpl::SmartBuffer &buf, bool &fullyWritten,
	              bool flush = true);

	void enableWriteCbIfNeeded(void);
	void enableReadCb(void);
	bool isExistingDir(const std::string &dirname, bool &hasError);
	bool makeBasedirIfNeeded(const std::string &baseDir);
	bool deleteFileIfExists(const std::string &path);
	bool checkGIOStatus(GIOStatus stat, GError *error);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

