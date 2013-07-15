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

#ifndef PipeUtils_h
#define PipeUtils_h

#include <string>
#include <stdint.h>

class PipeUtils {
public:
	PipeUtils(void);
	virtual ~PipeUtils();
	bool makeFileAndOpen(int flags);
	bool makeFileInTmpAndOpenForRead();
	bool makeFileInTmpAndOpenForWrite();
	const std::string getPath(void) const;
	bool openForRead(const std::string &path);
	bool openForWrite(const std::string &path);
	bool send(size_t size, void *buf);

	/**
	 * receive data from pipe.
	 *
	 * @param size A received data size.
	 * @param buf  A buffer to store the received data.
	 * @param timeout
	 * A timeout in msec. If this parameter is zero, this function
	 * waits forever until the reques size of data is received.
	 *
	 * @return
	 * If any error happens or timeout occurs, false is returned.
	 * Otherwise true.
	 */
	bool recv(size_t size, void *buf, size_t timeout = 0);

protected:
	std::string makePipeFileInTmpDir(int id);
	bool makeFileInTmpAndOpen(int flags, int id);
	bool openPipe(int flags);
	bool waitForRead(size_t timeout);
	bool readWithErrorCheck(size_t size, uint8_t *buf, size_t &index);

private:
	int         m_fd;
	std::string m_path;
};

#endif // PipeUtils_h
