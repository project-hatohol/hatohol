/*
 * Copyright (C) 2014 Project Hatohol
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

#ifndef RestResourceHost_h
#define RestResourceHost_h

#include "FaceRestPrivate.h"

struct RestResourceHost : public FaceRest::ResourceHandler
{
	typedef void (RestResourceHost::*HandlerFunc)(void);

	static void registerFactories(FaceRest *faceRest);

	RestResourceHost(FaceRest *faceRest, HandlerFunc handler);
	virtual ~RestResourceHost();

	virtual void handle(void) override;

	void handlerGetOverview(void);
	void handlerGetHost(void);
	void handlerGetTrigger(void);
	void handlerGetEvent(void);
	void handlerIncident(void);
	void handlerPutIncident(void);
	void handlerGetHostgroup(void);
	void handlerGetItem(void);
	void replyGetItem(void);
	void handlerGetHistory(void);
	void itemFetchedCallback(Closure0 *closure);
	void historyFetchedCallback(Closure1<HistoryInfoVect> *closure,
				    const HistoryInfoVect &historyInfoVect);

	static HatoholError parseEventParameter(EventsQueryOption &option,
						GHashTable *query);
	static bool parseExtendedInfo(const std::string &extendedInfo,
	                              std::string &extendedInfoValue);

	static const char *pathForOverview;
	static const char *pathForHost;
	static const char *pathForTrigger;
	static const char *pathForEvent;
	static const char *pathForIncident;
	static const char *pathForItem;
	static const char *pathForHistory;
	static const char *pathForHostgroup;

	HandlerFunc m_handlerFunc;
};

struct RestResourceHostFactory : public FaceRest::ResourceHandlerFactory
{
	RestResourceHostFactory(FaceRest *faceRest,
				RestResourceHost::HandlerFunc handler);
	virtual FaceRest::ResourceHandler *createHandler(void) override;

	RestResourceHost::HandlerFunc m_handlerFunc;
};

#endif // RestResourceHost_h
