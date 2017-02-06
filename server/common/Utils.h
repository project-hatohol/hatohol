/*
 * Copyright (C) 2013-2015 Project Hatohol
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
#include <cstdlib>
#include <vector>
#include <string>
#include <typeinfo>
#include <functional>
#include <inttypes.h>
#include <glib.h>
#include <StringUtils.h>
#include <execinfo.h>
#include <libsoup/soup.h>
#include "Params.h"
#include "Monitoring.h"

#ifndef GLIB_VERSION_2_32
#define G_SOURCE_REMOVE FALSE
#endif

#ifndef SOUP_VERSION_2_32
#define soup_uri_get_scheme(uri) (uri->scheme)
#define soup_uri_get_host(uri) (uri->host)
#define soup_uri_get_port(uri) (uri->port)
#define soup_uri_get_user(uri) (uri->user)
#define soup_uri_get_password(uri) (uri->password)
#define soup_uri_get_path(uri) (uri->path)
#endif

#ifndef SOUP_URI_IS_VALID
#define SOUP_URI_IS_VALID(uri) ((uri) && (uri)->scheme && (uri)->path)
#endif

class FormulaElement;

static const guint INVALID_EVENT_ID = -1;

class Utils {
public:
	static void init(void);
	static std::string makeDemangledStackTraceLines(void **trace, int num);
	static void assertNotNull(const void *ptr);
	static std::string demangle(const std::string &str);
	static bool isValidPort(int port, bool showErrorMsg = true);
	static std::string getExtension(const std::string &path);
	static bool validateJSMethodName(const std::string &name,
	                                 std::string &errorMessage);
	static std::string getSelfExeDir(void);
	static std::string getStringFromGIOCondition(GIOCondition condition);

	static guint setGLibIdleEvent(GSourceFunc func, gpointer data = NULL,
	                              GMainContext *context = NULL);

	/**
	 * Regisiter a GLib's timer.
	 *
	 * @param interval A timer interval in millisecond.
	 * @param func     A timer callback function.
	 * @param data     Any data passed to 'func'.
	 * @param context  A GMainContext. If NULL, the default context is used.
	 *
	 * @return A GSource ID.
	 */
	static guint setGLibTimer(guint interval,
	                          GSourceFunc func, gpointer data = NULL,
	                          GMainContext *context = NULL);

	/**
	 * Watch a file descriptor in the GLIB event loop.
	 *
	 * @param fd
	 * A file descriptor to be watched.
	 *
	 * @param events
	 * A combination of GIOCondition.
	 * It is typically G_IO_IN | G_IO_HUP | G_IO_ERR for reading, and
	 * G_IO_OUT | G_IO_ERR for for writing.
	 *
	 * @param func
	 * A function called when an event occurs.
	 *
	 * @param data
	 * A private data passed to the callback function.
	 *
	 * @param context
	 * A GLIB main loop context. If it's NULL, the default context is used.
	 *
	 * @return An event tag.
	 */
	static guint watchFdInGLibMainLoop(
	  int fd, gushort events, GSourceFunc func, gpointer data = NULL,
	  GMainContext *context = NULL);

	/**
	 * execute a function on the specified GLIB event loop.
	 *
	 * @param func
	 * A function to be executed.
	 *
	 * @param data
	 * A pointer that is passed to 'func' as an argument.
	 *
	 * @param syncType
	 * If this is SYNC, this function returns after 'func' is completed.
	 * If this is ASYNC, this function returns immediately.
	 *
	 * @param context
	 * A GMainContext on which the function is execuetd. If this is NULL,
	 * the default context is used.
	 *
	 * @return
	 * An event ID of the scheduled task. When syncType is SYNC,
	 * INVALID_EVENT_ID is always returned.
	 */
	static guint executeOnGLibEventLoop(
	  void (*func)(gpointer data), gpointer data = NULL,
	  SyncType syncType = SYNC, GMainContext *context = NULL);

	template<typename T>
	static guint executeOnGLibEventLoop(
	  void (*func)(T *data), T *data = NULL, SyncType syncType = SYNC,
	  GMainContext *context = NULL)
	{
		return executeOnGLibEventLoop(
		  reinterpret_cast<void (*)(gpointer)>(func),
		  static_cast<gpointer>(data), syncType, context);
	}

	template<typename T>
	static guint executeOnGLibEventLoop(T &obj, SyncType syncType = SYNC,
	                                    GMainContext *context = NULL)
	{
		struct Task {
			static void run(T *obj) {
				(*obj)();
			}
		};
		return executeOnGLibEventLoop(Task::run, &obj,
		                              syncType, context);
	}

	template<typename T>
	static guint deleteOnGLibEventLoop(T *obj, SyncType syncType = SYNC,
	                                   GMainContext *context = NULL)
	{
		struct Task {
			static void run(T *obj) {
				delete obj;
			}
		};
		return executeOnGLibEventLoop<T>(Task::run, obj,
		                                 syncType, context);
	}

	/**
	 * remove a GLIB's event.
	 *
	 * @param tag A event tag.
	 *
	 * @param syncType 
	 * If this is SYNC, this function returns after the event source is
	 * completely removed.
	 * If this is ASYNC, this function returns immediately.
	 *
	 * @param context A GLibMainContext.
	 *
	 * @return true if the event was successfuly removed. Otherwise false.
	 */
	static bool removeEventSourceIfNeeded(guint tag,
	                                      SyncType syncType = SYNC,
	                                      GMainContext *context = NULL);

	/**
	 * compute a SHA256.
	 *
	 * @param data
	 * A source string to be computed.
	 *
	 * @return A SHA256 string.
	 */
	static std::string sha256(const std::string &data);

	static pid_t getThreadId(void);

	/**
	 * Return information about the program using a port.
	 * Tihs method internally uses a 'lsof' command.
	 */
	static std::string getUsingPortInfo(const int &port);

	/**
	 * Call g_source_remove() and show an error message if it fails.
	 *
	 * @param
	 * A event tag passed to g_source_remove(). If this parameters is
	 * INVALID_EVENT_ID, the function immediately returns
	 * with doing nothing.
	 *
	 * @return
	 * If the tag is INVALID_EVENT_ID or the removal is sucessfully done,
	 * true is returned. Otherwise, false is returned.
	 */
	static bool removeGSourceIfNeeded(const guint &tag);

	static void flushPendingGLibEvents(GMainContext *context = NULL);

	/**
	 * Check if an string is a valid IPv4 or v6 address.
	 *
	 * @param ipAddress A string to be checked.
	 * @return true if the string is valid, or false.
	 */
	static bool isValidIPAddress(const std::string &ipAddress);

	/**
	 * Check if an string is a valid IPv4 address.
	 *
	 * @param ipAddress A string to be checked.
	 * @return true if the string is valid, or false.
	 */
	static bool isValidIPv4Address(const std::string &ipAddress);

	/**
	 * Check if an string is a valid IPv6 address.
	 *
	 * @param ipAddress A string to be checked.
	 * @return true if the string is valid, or false.
	 */
	static bool isValidIPv6Address(const std::string &ipAddress);

	/**
	 * Check whether the string is a valid URI or not.
	 *
	 * @param uri A string to be checked.
	 * @return true if the string is valid, otherwise false.
	 */
	static bool isValidURI(const std::string &uri);

	/**
	 * Convert a decimal number string into a number.
	 *
	 * @param dest A destination.
	 * @param src  A srouce string.
	 */
	static void conv(uint64_t &dest, const std::string &src);

	static void foreachTriggerStatus(
	  std::function<void(const TriggerStatusType &)> func);

	static void foreachTriggerStatusDouble(
	  std::function<void(const TriggerStatusType &,
	                     const TriggerStatusType &)> func);

protected:
	static std::string makeDemangledStackTraceString(
	  const std::string &stackTraceLine);
};

#define TRMSG(msg, fmt, ...) \
do { \
  void *trace[128]; \
  int n = backtrace(trace, ARRAY_SIZE(trace)); \
  msg = mlpl::StringUtils::sprintf("<%s:%d> " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
  msg += Utils::makeDemangledStackTraceLines(trace, n); \
} while (0)

#define TYPE_NAME(X)            typeid(X).name()
#define DEMANGLED_TYPE_NAME(X)  Utils::demangle(TYPE_NAME(X)).c_str()

#define ARRAY_SIZE(a)	(sizeof(a)/sizeof(a[0]))

