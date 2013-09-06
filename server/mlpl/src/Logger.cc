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

#include <cstdio>
#include <cstdlib>
#include <stdarg.h>
#include <string>
#include <syslog.h>
using namespace std;
#include "Logger.h"
using namespace mlpl;
#include <string.h>
#include "StringUtils.h"

static const char* LogHeaders [MLPL_NUM_LOG_LEVEL] = {
	"BUG", "CRIT", "ERR", "WARN", "INFO", "DBG",
};

LogLevel Logger::m_currLogLevel = MLPL_LOG_LEVEL_NOT_SET;
pthread_rwlock_t Logger::m_rwlock = PTHREAD_RWLOCK_INITIALIZER;
bool Logger::syslogoutputFlag = true;
ReadWriteLock Logger::lock;
const char *Logger::LEVEL_ENV_VAR_NAME = "MLPL_LOGGER_LEVEL";

// ----------------------------------------------------------------------------
// Public methods
// ----------------------------------------------------------------------------
void Logger::log(LogLevel level, const char *fileName, int lineNumber,
                 const char *fmt, ...)
{
	
	fprintf(stderr, "[%s] <%s:%d> ", LogHeaders[level], fileName,
	        lineNumber);
	
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	lock.readLock();
	if (syslogoutputFlag) {
		string msg = 
		   StringUtils::sprintf("[%s] <%s:%d> ", LogHeaders[level],
		                        fileName, lineNumber);

		static size_t bufSize = 256;
		char tmp[bufSize];
		memset(tmp, '\0', sizeof(tmp));
		va_start(ap, fmt);
		vsnprintf(tmp, bufSize, fmt, ap);
		va_end(ap);
		msg += tmp;

		openlog(fileName, LOG_CONS | LOG_PID, LOG_USER);
		syslog(LOG_INFO, "%s", msg.c_str());
		closelog();
	}
	lock.unlock();
}

// ----------------------------------------------------------------------------
// Protected methods
// ----------------------------------------------------------------------------
bool Logger::shouldLog(LogLevel level)
{
	bool ret = false;
	pthread_rwlock_rdlock(&m_rwlock);
	if (m_currLogLevel == MLPL_LOG_LEVEL_NOT_SET) {
		pthread_rwlock_unlock(&m_rwlock);
		setCurrLogLevel();
		pthread_rwlock_rdlock(&m_rwlock);
	}
	if (level <= m_currLogLevel)
		ret = true;
	pthread_rwlock_unlock(&m_rwlock);
	return ret;
}

void Logger::enableSyslogOutput(void)
{
	lock.writeLock();
	syslogoutputFlag = true;
	lock.unlock();
}

void Logger::disableSyslogOuputput(void)
{
	lock.writeLock();
	syslogoutputFlag = false;
	lock.unlock();
}

void Logger::setCurrLogLevel(void)
{
	pthread_rwlock_wrlock(&m_rwlock);
	if (m_currLogLevel != MLPL_LOG_LEVEL_NOT_SET) {
		pthread_rwlock_unlock(&m_rwlock);
		return;
	}

	char *env = getenv(LEVEL_ENV_VAR_NAME);
	if (!env) {
		m_currLogLevel = MLPL_LOG_INFO;
		pthread_rwlock_unlock(&m_rwlock);
		return;
	}
	string envStr = env;
	if (envStr == "DBG")
		m_currLogLevel = MLPL_LOG_DBG;
	else if (envStr == "INFO")
		m_currLogLevel = MLPL_LOG_INFO;
	else if (envStr == "WARN")
		m_currLogLevel = MLPL_LOG_WARN;
	else if (envStr == "ERR")
		m_currLogLevel = MLPL_LOG_ERR;
	else if (envStr == "CRIT")
		m_currLogLevel = MLPL_LOG_CRIT;
	else if (envStr == "BUG")
		m_currLogLevel = MLPL_LOG_BUG;
	else {
		log(MLPL_LOG_WARN, __FILE__, __LINE__,
		    "Unknown level: %s", env);
		m_currLogLevel = MLPL_LOG_INFO;
	}
	pthread_rwlock_unlock(&m_rwlock);
}
