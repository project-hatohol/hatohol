/*
 * Copyright (C) 2013-2014 Project Hatohol
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
#include <string>
#include "StringUtils.h"
#include "Params.h"
#include "JSONParser.h"
#include "HatoholError.h"

typedef std::map<std::string, std::string> StringMap;
typedef StringMap::iterator                StringMapIterator;
typedef StringMap::const_iterator          StringMapConstIterator;

struct RequestArg {
	// input
	std::string        url;
	std::string        callbackName;
	std::string        request;
	StringMap          parameters;
	mlpl::StringVector headers;
	UserIdType         userId;

	// output
	std::string        response;
	int                httpStatusCode;
	std::string        httpReasonPhrase;
	mlpl::StringVector responseHeaders;

	RequestArg(const std::string &_url, const std::string &_cbname = "")
	: url(_url),
	  callbackName(_cbname),
	  request("GET"),
	  userId(INVALID_USER_ID),
	  httpStatusCode(-1)
	{
	}
};

void startFaceRest(void);
void stopFaceRest(void);
std::string makeSessionIdHeader(const std::string &sessionId);

void getServerResponse(RequestArg &arg);
JSONParser *getResponseAsJSONParser(RequestArg &arg);
JSONParser *getServerResponseAsJSONParserWithFailure(RequestArg &arg);

void _assertValueInParser(JSONParser *parser,
			  const std::string &member,
			  const bool expected);
void _assertValueInParser(JSONParser *parser,
			  const std::string &member,
			  uint32_t expected);
void _assertValueInParser(JSONParser *parser,
			  const std::string &member,
			  int expected);
void _assertValueInParser(JSONParser *parser,
			  const std::string &member,
			  uint64_t expected);
void _assertValueInParser(JSONParser *parser,
			  const std::string &member,
			  const timespec &expected);
void _assertValueInParser(JSONParser *parser,
			  const std::string &member,
			  const std::string &expected);
#define assertValueInParser(P,M,E) cut_trace(_assertValueInParser(P,M,E))

void _assertStartObject(JSONParser *parser, const std::string &keyName);
#define assertStartObject(P,K) cut_trace(_assertStartObject(P,K))

void _assertNoValueInParser(JSONParser *parser,
			    const std::string &member);
#define assertNoValueInParser(P,M) cut_trace(_assertNoValueInParser(P,M))

void _assertNullInParser(JSONParser *parser, const std::string &member);
#define assertNullInParser(P,M) cut_trace(_assertNullInParser(P,M))

void _assertErrorCode(JSONParser *parser,
		      const HatoholErrorCode &expectCode = HTERR_OK);
#define assertErrorCode(P, ...) cut_trace(_assertErrorCode(P, ##__VA_ARGS__))

void _assertAddRecord(const StringMap &params, const std::string &url,
                      const UserIdType &userId = INVALID_USER_ID,
                      const HatoholErrorCode &expectCode = HTERR_OK,
                      uint32_t expectedId = 1);
void _assertUpdateRecord(const StringMap &params, const std::string &baseUrl,
                         uint32_t targetId = 1,
                         const UserIdType &userId = INVALID_USER_ID,
                         const HatoholErrorCode &expectCode = HTERR_OK);

void assertServersIdNameHashInParser(JSONParser *parser);

