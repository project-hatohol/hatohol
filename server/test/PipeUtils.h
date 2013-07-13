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

protected:
	std::string makePipeFileInTmpDir(int id);
	bool makeFileInTmpAndOpen(int flags, int id);
	bool openPipe(int flags);

private:
	int         m_fd;
	std::string m_path;
};

#endif // PipeUtils_h
