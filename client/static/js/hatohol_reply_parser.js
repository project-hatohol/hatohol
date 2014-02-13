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

// ---------------------------------------------------------------------------
// HatohoReplyParser
// ---------------------------------------------------------------------------
var REPLY_STATUS = {
  OK: 0,
  NULL_OR_UNDEFINED:        1,
  NOT_FOUND_API_VERSION:    2,
  UNSUPPORTED_API_VERSION:  3,
  NOT_FOUND_ERROR_CODE:     4,
  ERROR_CODE_IS_NOT_OK:     5,
  NOT_FOUND_ERROR_MESSAGE:  6,
  NOT_FOUND_SESSION_ID:     7,
};

var HatoholReplyParser = function(reply) {

  this.stat = REPLY_STATUS.OK;
  this.optionMessage = "";

  if (!reply) {
    this.stat = REPLY_STATUS.NULL_OR_UNDEFINED;
    return;
  }
  
  // API version
  if (!("apiVersion" in reply)) {
    this.stat = REPLY_STATUS.NOT_FOUND_API_VERSION;
    return;
  }
  if (reply.apiVersion != hatohol.FACE_REST_API_VERSION) {
    this.stat = REPLY_STATUS.UNSUPPORTED_API_VERSION;
    return;
  }

  // error code
  if (!("errorCode" in reply)) {
    this.stat = REPLY_STATUS.NOT_FOUND_ERROR_CODE;
    return;
  }
  this.errorCode = reply.errorCode;
  this.errorMessage = reply.errorMessage;
  if ("optionMessage" in reply)
    this.optionMessage = reply.optionMessage;
  if (reply.errorCode != hatohol.HTERR_OK) {
    this.stat = REPLY_STATUS.ERROR_CODE_IS_NOT_OK;
    return;
  }
};

HatoholReplyParser.prototype.getStatus = function() {
  return this.stat;
};

HatoholReplyParser.prototype.getErrorCode = function() {
  return this.errorCode;
};

HatoholReplyParser.prototype.getErrorMessage = function() {
  // Build a more friendly message than a server's message if it's needed.
  switch (this.errorCode) {
  case hatohol.HTERR_AUTH_FAILED:
    return gettext("Invalid user name or password.");
  }

  // Return a pre-defined & translated error message.
  if (hatohol.errorMessages[this.errorCode])
    return hatohol.errorMessages[this.errorCode];

  // Return an error message which is sent from the server.
  if (this.errorMessage)
    return this.errorMessage;

  // No error message is found. Return the raw error code.
  var errorCodeName;
  if ((errorCodeName = this.getErrorCodeName())) {
    return gettext("Error: ") + this.errorCode + ", " + errorCodeName;
  } else {
    return gettext("Unknown error: ") + this.errorCode;
  }
};

HatoholReplyParser.prototype.getMessage = function() {
  var message;

  switch (this.stat) {
  case REPLY_STATUS.OK:
    message = gettext("OK.");
    break;
  case REPLY_STATUS.NULL_OR_UNDEFINED:
    message = gettext("Null or undefined.");
    break;
  case REPLY_STATUS.NOT_FOUND_ERROR_CODE:
    message = gettext("Not found errorCode.");
    break;
  case REPLY_STATUS.ERROR_CODE_IS_NOT_OK:
  case REPLY_STATUS.NOT_FOUND_ERROR_MESSAGE:
    message = this.getErrorMessage();
    break;
  case REPLY_STATUS.NOT_FOUND_SESSION_ID:
    message = gettext("Not found sessionId.");
    break;
  default:
    message = gettext("Unknown status: ") + this.stat;
    break;
  }

  if (this.optionMessage)
    message += gettext(": ") + this.optionMessage;

  return message;
};

HatoholReplyParser.prototype.getErrorCodeName = function() {
  var key, code = this.getErrorCode();
  for (key in hatohol) {
    if (key.match(/^HTERR_/) && hatohol[key] == code)
      return key;
  }
  return null;
};

// ---------------------------------------------------------------------------
// HatoholLoginReplyParser
// ---------------------------------------------------------------------------
var HatoholLoginReplyParser = function(reply) {
  HatoholReplyParser.apply(this, [reply]);
  if (this.getStatus() != REPLY_STATUS.OK)
    return;
  if (!("sessionId" in reply))
    this.stat = REPLY_STATUS.NOT_FOUND_SESSION_ID;
  this.sessionId = reply.sessionId;
};

HatoholLoginReplyParser.prototype = Object.create(HatoholReplyParser.prototype);
HatoholLoginReplyParser.prototype.constructor = HatoholLoginReplyParser;

HatoholLoginReplyParser.prototype.getSessionId = function() {
  return this.sessionId;
};

