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

#ifndef Logger_h
#define Logger_h

#include <pthread.h>
#include "ReadWriteLock.h"
namespace hfl {

enum LogLevel {
	HFL_LOG_BUG,
	HFL_LOG_CRIT,
	HFL_LOG_ERR,
	HFL_LOG_WARN,
	HFL_LOG_INFO,
	HFL_LOG_DBG,

	HFL_NUM_LOG_LEVEL,
	HFL_LOG_LEVEL_NOT_SET,
};

class Logger {
public:
	static const char *LEVEL_ENV_VAR_NAME;
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
private:
	static LogLevel m_currLogLevel;
	static pthread_rwlock_t m_rwlock;
	static bool syslogoutputFlag;
	static ReadWriteLock lock;
	static bool syslogConnected;
};

} // namespace hfl

#define HFL_P(LOG_LV, FMT, ...) \
do { \
  if (hfl::Logger::shouldLog(LOG_LV)) \
    hfl::Logger::log(LOG_LV, __FILE__, __LINE__, FMT, ##__VA_ARGS__); \
} while (0)

#define HFL_DBG(FMT,  ...) HFL_P(hfl::HFL_LOG_DBG,  FMT, ##__VA_ARGS__)
#define HFL_INFO(FMT, ...) HFL_P(hfl::HFL_LOG_INFO, FMT, ##__VA_ARGS__)
#define HFL_WARN(FMT, ...) HFL_P(hfl::HFL_LOG_WARN, FMT, ##__VA_ARGS__)
#define HFL_ERR(FMT,  ...) HFL_P(hfl::HFL_LOG_ERR,  FMT, ##__VA_ARGS__)
#define HFL_CRIT(FMT, ...) HFL_P(hfl::HFL_LOG_CRIT, FMT, ##__VA_ARGS__)
#define HFL_BUG(FMT,  ...) HFL_P(hfl::HFL_LOG_BUG,  FMT, ##__VA_ARGS__)

#endif // Logger_h

