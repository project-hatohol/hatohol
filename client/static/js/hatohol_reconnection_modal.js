/*
 * Copyright (C) 2015 Project Hatohol
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

var HatoholReconnectModal = function(retryFunc, errorMsg) {
  var self = this;
  var DEFAULT_RETRY_INTERVAL = 30;
  self.timeToReconnect = DEFAULT_RETRY_INTERVAL;

  HatoholModal.apply(self, [{
    "id": "reconnectionModal",
    "title": gettext("Connection Error"),
    "body": $(getBodyHTML()),
  }]);

  setTimeout(function() {self.countdown();}, 1000);

  function getBodyHTML() {
    var s = "<p>";
    if (errorMsg) {
      s += errorMsg;
      s += "<hr>";
    }
    s += gettext("Try to reconnect after ") + self.timeToReconnect + gettext(" sec.");
    s += "</p>";
    return s;
  };

  self.countdown = function() {
    self.timeToReconnect -= 1;
    if (self.timeToReconnect == 0) {
      self.close(retryFunc);
    } else {
      self.updateBody($(getBodyHTML()));
      setTimeout(function() {self.countdown();}, 1000);
    }
  };

};

HatoholReconnectModal.prototype = Object.create(HatoholModal.prototype);
HatoholReconnectModal.prototype.constructor = HatoholReconnectModal;

