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

var REPLY_STATUS = {
  OK: 0,
  NULL_OR_UNDEFINED: 1,
  NOT_FOUND_RESULT:  2,
  RESULT_IS_FALSE:   3,
  RESULT_IS_FALSE_BUT_NOT_FOUND_MSG: 4,
};

var HatoholReplyParser = function(reply) {

  this.stat = REPLY_STATUS.OK;
  this.errorMessage = "";

  if (!reply) {
    this.stat = REPLY_STATUS.NULL_OR_UNDEFIND;
  } else if (!("result" in reply)) {
    this.stat = REPLY_STATUS.NOT_FOUND_RESULT;
  } else if (!reply.result) {
    if ("message" in reply) {
      this.stat = REPLY_STATUS.RESULT_IS_FALSE;
      this.errorMessage = result.message;
    } else {
      this.stat = REPLY_STATUS.RESULT_IS_FALSE_BUT_NOT_FOUND_MSG;
    }
  }
}

HatoholReplyParser.prototype.getStatus = function() {
  return this.stat;
}

HatoholReplyParser.prototype.getStatusMessage = function() {
  switch (this.stat) {
  case REPLY_STATUS.OK:
    return "OK.";
  case REPLY_STATUS.NULL_OR_UNDEFINED:
    return "Null or undefined.";
  case REPLY_STATUS.NOT_FOUND_RESULT:
    return "Not found: result.";
  case REPLY_STATUS.RESULT_IS_FALSE:
    return "Result is false: " + this.errorMessage;
  case REPLY_STATUS.RESULT_IS_FALSE_BUT_NOT_FOUND_MSG:
    return "Result is false, but message is not found.";
  }
  return "Unknown status: " + this.stat;
}
