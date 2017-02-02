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

#include <StringUtils.h>
#include <Logger.h>
#include <Mutex.h>
#include <Reaper.h>
using namespace mlpl;

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cxxabi.h>
#include <stdexcept>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <syscall.h>
#include <limits.h>
#include <arpa/inet.h>
#include "Utils.h"
#include "HatoholException.h"
using namespace std;

const static size_t SIZE_JS_METHOD_VALID_CODE_MAP = 0x100;
static bool g_jsMethodValidCodeMap[SIZE_JS_METHOD_VALID_CODE_MAP];


// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void Utils::init(void)
{
	for (size_t i = 0; i < SIZE_JS_METHOD_VALID_CODE_MAP; i++)
		g_jsMethodValidCodeMap[i] = false;
	for (size_t i = 'A'; i <= 'Z'; i++)
		g_jsMethodValidCodeMap[i] = true;
	for (size_t i = 'a'; i <= 'z'; i++)
		g_jsMethodValidCodeMap[i] = true;
	for (size_t i = '0'; i <= '9'; i++)
		g_jsMethodValidCodeMap[i] = true;
	g_jsMethodValidCodeMap[static_cast<size_t>('_')] = true;
}

string Utils::makeDemangledStackTraceLines(void **trace, int num)
{
	char **symbols = backtrace_symbols(trace, num);
	string str;
	for (int i = 0; i < num; i++) {
		string line = symbols[i];
		str += makeDemangledStackTraceString(line);
		str += "\n";
	}
	free(symbols);
	return str;
}

void Utils::assertNotNull(const void *ptr)
{
	if (ptr)
		return;
	string msg;
	TRMSG(msg, "pointer: NULL.");
	throw logic_error(msg);
}

string Utils::demangle(const string &str)
{
	int status;
	char *demangled = abi::__cxa_demangle(str.c_str(), 0, 0, &status);
	string demangledStr;
	if (demangled) {
		demangledStr = demangled;
		free(demangled);
	}
	return demangledStr;
}

uint64_t Utils::getCurrTimeAsMicroSecond(void)
{
	struct timeval tv;
	if (gettimeofday(&tv, NULL) == -1) {
		MLPL_ERR("Failed to call gettimeofday: %d\n", errno);
		return 0;
	}
	uint64_t currTime = tv.tv_usec;
	currTime += 1000 * 1000 * tv.tv_sec;
	return currTime;
}

bool Utils::isValidPort(int port, bool showErrorMsg)
{
	if (port < 0 || port >= 65536) {
		if (showErrorMsg)
			MLPL_ERR("invalid port: %d\n", port);
		return false;
	}
	return true;
}

bool Utils::validateJSMethodName(const string &name, string &errorMessage)
{
	if (name.empty()) {
		errorMessage = "empty name.";
		return false;
	}

	if (name[0] >= '0' && name[0] <= '9') {
		errorMessage = "begins from a number.";
		return false;
	}

	for (size_t i = 0; i < name.size(); i++) {
		int code = name[i];
		if (g_jsMethodValidCodeMap[code])
			continue;
		errorMessage = StringUtils::sprintf(
		  "invalid character code: %02x at index: %zd", code, i);
		return false;
	}

	return true;
}

string Utils::getExtension(const string &path)
{
	string ext;
	int len = path.size();
	for (int i = len-1; i >= 0; i--) {
		if (path[i] != '.')
			continue;
		ext = string(path, i+1);
		break;
	}
	return ext;
}

string Utils::getSelfExeDir(void)
{
	char buf[PATH_MAX];
	void *p;
	ssize_t bytesRead = readlink("/proc/self/exe", buf, sizeof(buf));

	if (bytesRead == -1) {
		THROW_HATOHOL_EXCEPTION(
		  "Failed to readlink(\"/proc/self/exe\"): %s",
		  g_strerror(errno));
	}
	p = memrchr(buf, '/', bytesRead);
	if (!p) {
		// I don't think this could happen.
		THROW_HATOHOL_EXCEPTION(
		  "readlink(\"/proc/self/exe\") points to non-absolute path."
		);
	}
	return p != buf ? string(buf, (char *)p - buf) : "/";
}

string Utils::getStringFromGIOCondition(GIOCondition condition)
{
	struct CondStruct {
		GIOCondition condition;
		const char *word;
	};
	const CondStruct condStruct[] = {
	   {G_IO_IN, "G_IO_IN"},
	   {G_IO_OUT, "G_IO_OUT"},
	   {G_IO_PRI, "G_IO_PRI"},
	   {G_IO_ERR, "G_IO_ERR"},
	   {G_IO_HUP, "G_IO_HUP"},
	   {G_IO_NVAL, "G_IO_NVAL"},
	};
	const size_t numCondStruct = ARRAY_SIZE(condStruct);

	string str;
	for (size_t i = 0; i < numCondStruct; i++) {
		const CondStruct &cs = condStruct[i];
		if (!(condition & cs.condition))
			continue;
		if (!str.empty())
			str += " ";
		str += cs.word;
	}
	return str;
}

guint Utils::setGLibIdleEvent(GSourceFunc func, gpointer data,
                              GMainContext *context)
{
	if (!context)
		context = g_main_context_default();
	GSource *source = g_idle_source_new();
	g_source_set_callback(source, func, data, NULL);
	guint id = g_source_attach(source, context);
	g_source_unref(source);
	return id;
}

guint Utils::setGLibTimer(guint interval,
                          GSourceFunc func, gpointer data,
                          GMainContext *context)
{
	if (!context)
		context = g_main_context_default();
	GSource *source = g_timeout_source_new(interval);
	g_source_set_callback(source, func, data, NULL);
	guint id = g_source_attach(source, context);
	g_source_unref(source);
	return id;
}

guint Utils::watchFdInGLibMainLoop(int fd, gushort events,
                                   GSourceFunc func, gpointer data,
                                   GMainContext *context)
{
	struct WatchFdSource {
		GSource gsource;
		GPollFD gPollFd;
	};

	struct WatchFdFuncs {
		static gboolean prepare(GSource *source, gint *timeout_)
		{
			return FALSE;
		}

		static gboolean check(GSource *source)
		{
			WatchFdSource *watchSrc =
			  reinterpret_cast<WatchFdSource *>(source);
			const GPollFD &gPollFd = watchSrc->gPollFd;
			return gPollFd.revents & gPollFd.events;
		}

		static gboolean dispatch(GSource *source,
		                         GSourceFunc callback,
		                         gpointer user_data)
		{
			return (*callback)(user_data);
		}

		static void finalize(GSource *source)
		{
		}
	};

	static GSourceFuncs watchFdSourceFuncs = {
		WatchFdFuncs::prepare,
		WatchFdFuncs::check,
		WatchFdFuncs::dispatch,
		WatchFdFuncs::finalize,
	};

	if (!context)
		context = g_main_context_default();
	GSource *source = g_source_new(&watchFdSourceFuncs,
	                               sizeof(WatchFdSource));
	WatchFdSource *watchFdSrc = reinterpret_cast<WatchFdSource *>(source);
	watchFdSrc->gPollFd.fd = fd;
	watchFdSrc->gPollFd.events = events;
	watchFdSrc->gPollFd.revents = 0;

	g_source_add_poll(source, &watchFdSrc->gPollFd); 
	g_source_set_callback(source, func, data, NULL);
	guint id = g_source_attach(source, context);
	g_source_unref(source);
	return id;
}

guint Utils::executeOnGLibEventLoop(
  void (*func)(gpointer), gpointer data, SyncType syncType,
  GMainContext *context)
{
	HATOHOL_ASSERT(syncType == SyncType::SYNC || syncType == SyncType::ASYNC,
	               "Invalid syncType: %d\n", syncType);
	struct IdleTask {
		void (*userFunc)(gpointer);
		gpointer    userData;
		Mutex       mutex;
		SyncType    syncType;

		static gboolean callbackGate(gpointer data) {
			IdleTask *obj = static_cast<IdleTask *>(data);
			obj->callback();
			return G_SOURCE_REMOVE;
		}

		void callback(void) {
			(*userFunc)(userData);
			if (syncType == SyncType::SYNC)
				mutex.unlock();
			else
				delete this;
		}
	};

	guint eventId = INVALID_EVENT_ID;

	// just call the function if the caller has the ownership
	// of the context.
	if (syncType == SyncType::SYNC && g_main_context_acquire(context)) {
		(*func)(data);
		g_main_context_release(context);
		return eventId;
	}

	IdleTask *task = new IdleTask();
	task->userFunc = func;
	task->userData = data;
	task->syncType = syncType;

	if (syncType == SyncType::SYNC)
		task->mutex.lock();

	eventId = setGLibIdleEvent(IdleTask::callbackGate, task, context);

	// wait for the completion
	if (syncType == SyncType::SYNC) {
		task->mutex.lock();
		delete task;
		eventId = INVALID_EVENT_ID;
	}
	return eventId;
}

struct RemoveEventTask {
	GSource *source;

	static void run(RemoveEventTask *obj)
	{
		g_source_destroy(obj->source);
	}
};

bool Utils::removeEventSourceIfNeeded(
  guint tag, SyncType syncType, GMainContext *context)
{
	if (tag == INVALID_EVENT_ID)
		return true;

	// We remove the event on the GLIB's main loop to avoid the race.
	// This is useful when this function is called on the thread other than
	// executing g_main_loop_run().
	RemoveEventTask task;
	task.source = g_main_context_find_source_by_id(context, tag);
	if (!task.source) {
		MLPL_ERR("Failed to find source. tag: %d, context: %p\n",
		         tag, context);
		return false;
	}
	executeOnGLibEventLoop<RemoveEventTask>(
	  RemoveEventTask::run, &task, syncType, context);
	return true;
}

string Utils::sha256(const string &data)
{
	gchar *sha256 = g_compute_checksum_for_string(G_CHECKSUM_SHA256,
	                                              data.c_str(), -1);
	HATOHOL_ASSERT(sha256, "checksum type may be wrong.");
	string shaStr = sha256;
	g_free(sha256);
	return shaStr;
}

pid_t Utils::getThreadId(void)
{
	return syscall(SYS_gettid);
}

string Utils::getUsingPortInfo(const int &port)
{
	gchar *stdOut = NULL;
	gchar *stdErr = NULL;
	gint exitStatus = 0;
	GError *error = NULL;;
	Reaper<void> stdOutReaper(stdOut, g_free);
	Reaper<void> stdErrReaper(stdErr, g_free);
	Reaper<GError> errorReaper(error, g_error_free);

	string cmd = StringUtils::sprintf("lsof -i:%d", port);
	gboolean succeeded =
	  g_spawn_command_line_sync(cmd.c_str(),
	                            &stdOut, &stdErr, &exitStatus, &error);
	if (!succeeded) {
		const gchar *errorMsg =
		   error ? error->message : "Unknown reason";
		MLPL_ERR("Failed to execute: %s: %s\n", cmd.c_str(), errorMsg);
		return "";
	}
	return StringUtils::sprintf(
	  "Self PID: %d, exit status: %d\n<<stdout>> %s\n<<stderr>> %s\n",
	  getpid(), exitStatus, stdOut, stdErr);
}

bool Utils::removeGSourceIfNeeded(const guint &tag)
{
	if (tag == INVALID_EVENT_ID)
		return true;

	if (!g_source_remove(tag)) {
		MLPL_ERR("Failed to remove source: %u\n", tag);
		return false;
	}
	return true;
}

void Utils::flushPendingGLibEvents(GMainContext *context)
{
	while (g_main_context_iteration(context, FALSE))
		;
}

bool Utils::isValidIPAddress(const string &ipAddress)
{
	if (isValidIPv4Address(ipAddress))
		return true;
	if (isValidIPv6Address(ipAddress))
		return true;
	return false;
}

bool Utils::isValidIPv4Address(const string &ipAddress)
{
	struct in_addr addr4;

	if (inet_pton(AF_INET, ipAddress.c_str(), &addr4) <= 0)
		return false;

	return true;
}

bool Utils::isValidIPv6Address(const string &ipAddress)
{
	struct in6_addr addr6;

	if (inet_pton(AF_INET6, ipAddress.c_str(), &addr6) <= 0)
		return false;

	return true;
}

bool Utils::isValidURI(const string &uri)
{
	SoupURI *soupURI = soup_uri_new(uri.c_str());
	bool valid = SOUP_URI_IS_VALID(soupURI);
	if (soupURI)
		soup_uri_free(soupURI);
	return valid;
}

void Utils::conv(uint64_t &dest, const string &src)
{
	int numConv = sscanf(src.c_str(), "%" PRIu64, &dest);
	if (numConv != 1) {
		THROW_HATOHOL_EXCEPTION_WITH_ERROR_CODE(
		  HTERR_INTERNAL_ERROR, "Failed to convert %s.\n", src.c_str());
	}
}

uint64_t Utils::sum(const string &num0, const uint64_t num1)
{
	uint64_t n;
	conv(n, num0);
	return n + num1;
}

void Utils::foreachTriggerStatus(function<void(const TriggerStatusType &)> func)
{
	for (int stat = 0; stat < NUM_TRIGGER_STATUS; stat++)
		func(static_cast<TriggerStatusType>(stat));
}

void Utils::foreachTriggerStatusDouble(
  function<void(const TriggerStatusType &, const TriggerStatusType &)> func)
{
	auto coreFunc = [&](const TriggerStatusType &a,
	                    const TriggerStatusType &b) {
		func(a, b);
	};

	auto innerLoop = [&](const TriggerStatusType &status) {
		foreachTriggerStatus(bind(coreFunc, status, placeholders::_1));
	};

	foreachTriggerStatus(innerLoop);
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
string Utils::makeDemangledStackTraceString(const string &stackTraceLine)
{
	StringVector stringsHead;
	StringUtils::split(stringsHead, stackTraceLine, '(');
	if (stringsHead.size() != 2)
		return stackTraceLine;

	StringVector stringsTail;
	StringUtils::split(stringsTail, stringsHead[1], ')');
	if (stringsTail.size() != 2)
		return stackTraceLine;

	string &libName    = stringsHead[0];
	string &symbolName = stringsTail[0];
	string &addr       = stringsTail[1];

	StringVector stringsSymbol;
	StringUtils::split(stringsSymbol, symbolName, '+');
	if (stringsSymbol.size() != 2)
		return stackTraceLine;

	string demangledStr = demangle(stringsSymbol[0]);
	string returnStr = libName;
	if (!demangledStr.empty())
		returnStr += ", " + demangledStr + " +" + stringsSymbol[1];
	returnStr += ", " + symbolName + addr;
	return returnStr;
}

