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

  HatoholModal.apply(self, [{
    "id": "reconnectionModal",
    "title": gettext("Connection Error"),
    "body": $(getBodyHTML()),
    "footer": $(getFooterHTML()),
  }]);

  // Only the object that created the modal first carreis out countdown.
  if (!self.isOwner())
    return;

  $("#reconn-retry-button").on("click", function() {
    self.close(retryFunc);
  });

  $("#reconn-cancel-button").on("click", function() {
    self.close();
  });

  function getBodyHTML() {
    var s = "<p>";
    s += errorMsg;
    s += "</p>";
    return s;
  }

  function getFooterHTML() {
    var calcelLabel = gettext("Cancel retrying");
    var s = '<button type="button" class="btn btn-primary"';
    s += 'id="reconn-cancel-button">' + calcelLabel + '</button>';
    var label = gettext("Retry now");
    s += '<button type="button" class="btn btn-primary"';
    s += 'id="reconn-retry-button">' + label + '</button>';
    return s;
  }
};

HatoholReconnectModal.prototype = Object.create(HatoholModal.prototype);
HatoholReconnectModal.prototype.constructor = HatoholReconnectModal;

