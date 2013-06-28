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
	static void log(LogLevel level,
	                const char *fileName, int lineNumber,
	                const char *fmt, ...);
	static bool shouldLog(LogLevel level);
protected:
	static void setCurrLogLevel(void);
private:
	static LogLevel m_currLogLevel;
	static pthread_rwlock_t m_rwlock;
};

#define MLPL_P(LOG_LV, FMT, ...) \
do { \
  if (Logger::shouldLog(LOG_LV)) \
    Logger::log(LOG_LV, __FILE__, __LINE__, FMT, ##__VA_ARGS__); \
} while (0)

#define MLPL_DBG(FMT,  ...) MLPL_P(MLPL_LOG_DBG,  FMT, ##__VA_ARGS__)
#define MLPL_INFO(FMT, ...) MLPL_P(MLPL_LOG_INFO, FMT, ##__VA_ARGS__)
#define MLPL_WARN(FMT, ...) MLPL_P(MLPL_LOG_WARN, FMT, ##__VA_ARGS__)
#define MLPL_ERR(FMT,  ...) MLPL_P(MLPL_LOG_ERR,  FMT, ##__VA_ARGS__)
#define MLPL_CRIT(FMT, ...) MLPL_P(MLPL_LOG_CRIT, FMT, ##__VA_ARGS__)
#define MLPL_BUG(FMT,  ...) MLPL_P(MLPL_LOG_BUG,  FMT, ##__VA_ARGS__)

} // namespace mlpl

#endif // Logger_h

