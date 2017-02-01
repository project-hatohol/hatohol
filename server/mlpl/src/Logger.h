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
#include <pthread.h>
#include <string>
#include "ReadWriteLock.h"

namespace mlpl {

enum LogLevel {
	MLPL_LOG_BUG,
	MLPL_LOG_CRIT,
	MLPL_LOG_ERR,
	MLPL_LOG_WARN,
	MLPL_LOG_INFO,
	MLPL_LOG_DBG,

	MLPL_NUM_LOG_LEVEL,
	MLPL_LOG_LEVEL_NOT_SET,
};

class Logger {
public:
	static const char *LEVEL_ENV_VAR_NAME;
	static const char *MLPL_LOGGER_FLAGS;
	static void log(LogLevel level,
	                const char *fileName, int lineNumber,
	                const char *fmt, ...)
		__attribute__((__format__ (__printf__, 4, 5)));
	static bool shouldLog(LogLevel level);
	static void enableSyslogOutput(void);
	static void disableSyslogOutput(void);
protected:
	static void setCurrLogLevel(void);
	static void connectSyslogIfNeeded(void);
	static std::string createHeader(LogLevel level, const char *fileName,
	                                int lineNumber, std::string extraInfoString);
	static std::string createExtraInfoString(void);
	static void setExtraInfoFlag(const char *extraInfoArg);
	static void addProcessId(std::string &extraInfoSrting);
	static void addThreadId(std::string &extraInfoSrting);
	static void addCurrentTime(std::string &extraInfoSrting);
	static void setupProcessId(void);
private:
	static LogLevel m_currLogLevel;
	static pthread_rwlock_t m_rwlock;
	static bool syslogoutputFlag;
	static ReadWriteLock lock;
	static bool syslogConnected;
	static bool extraInfoFlag[256];
	static pid_t pid;
	static __thread pid_t tid;
};

} // namespace mlpl

#define MLPL_P(LOG_LV, FMT, ...) \
do { \
  if (mlpl::Logger::shouldLog(LOG_LV)) \
    mlpl::Logger::log(LOG_LV, __FILE__, __LINE__, FMT, ##__VA_ARGS__); \
} while (0)

#define MLPL_DBG(FMT,  ...) MLPL_P(mlpl::MLPL_LOG_DBG,  FMT, ##__VA_ARGS__)
#define MLPL_INFO(FMT, ...) MLPL_P(mlpl::MLPL_LOG_INFO, FMT, ##__VA_ARGS__)
#define MLPL_WARN(FMT, ...) MLPL_P(mlpl::MLPL_LOG_WARN, FMT, ##__VA_ARGS__)
#define MLPL_ERR(FMT,  ...) MLPL_P(mlpl::MLPL_LOG_ERR,  FMT, ##__VA_ARGS__)
#define MLPL_CRIT(FMT, ...) MLPL_P(mlpl::MLPL_LOG_CRIT, FMT, ##__VA_ARGS__)
#define MLPL_BUG(FMT,  ...) MLPL_P(mlpl::MLPL_LOG_BUG,  FMT, ##__VA_ARGS__)

