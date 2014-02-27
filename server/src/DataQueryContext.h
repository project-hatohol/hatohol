/*
 * Copyright (C) 2014 Project Hatohol
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

#ifndef DataQueryContext_h
#define DataQueryContext_h

#include "UsedCountable.h"

/**
 * This class provides a function to share information for data query
 * among DataQueryOption and its subclasses.
 */
class DataQueryContext : public UsedCountable {
public:
	DataQueryContext(void);
	virtual ~DataQueryContext();

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // DataQueryContext_h
