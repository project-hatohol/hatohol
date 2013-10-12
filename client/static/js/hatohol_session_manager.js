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

var HatoholSessionManager = function() {
  var HATOHOL_SID_COOKIE_NAME = "hatoholSessionId";
  var HATOHOL_SID_COOKIE_MAX_AGE = 7 * 24 * 3600;

  return {
    get: function() {
      var cookie = document.cookie;
      var cookies = cookie.split(";");
      for(var i = 0; i < cookies.length; i++) {
        var hands = cookies[i].split("=");
        if (hands.length != 2)
          continue;
        if (hands[0] == HATOHOL_SID_COOKIE_NAME)
          return hands[1]
      }
      return null;
    },

    set: function(sessionId) {
      document.cookie = HATOHOL_SID_COOKIE_NAME + "=" + sessionId +
                        "; max-age=" + HATOHOL_SID_COOKIE_MAX_AGE;
    },

    deleteCookie: function() {
      var date = new Date();
      date.setTime(0);
      document.cookie = HATOHOL_SID_COOKIE_NAME + "=; expires=" +
                        date.toGMTString();
    }
  }
}();
