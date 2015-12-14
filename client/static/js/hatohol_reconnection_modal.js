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

var HatoholReconnectModalVars = {
  taskList: [],
};

var HatoholReconnectModal = function(retryFunc, errorMsg) {
  var self = this;
  var DEFAULT_RETRY_INTERVAL = 30;
  self.timeToReconnect = DEFAULT_RETRY_INTERVAL;

  HatoholModal.apply(self, [{
    "id": "reconnectionModal",
    "title": gettext("Connection Error"),
    "body": $(getBodyHTML()),
  }]);

  HatoholReconnectModalVars.taskList.push(retryFunc);
  // Only the object that created the modal first carreis out countdown.
  if (!self.isOwner())
    return;

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

  function retryAllTasks() {
    // Some tasks invoked in the following loop may push a task again and
    // cause an infinite loop. So we take the current tasks here and make
    // the new empty list in which new retry tasks are pushed.
    var taskList = HatoholReconnectModalVars.taskList;
    HatoholReconnectModalVars.taskList = [];
    for (var i = 0; i < taskList.length;i++)
        taskList[i]();
  };

  self.countdown = function() {
    self.timeToReconnect -= 1;
    if (self.timeToReconnect == 0) {
      self.close(retryAllTasks);
    } else {
      self.updateBody($(getBodyHTML()));
      setTimeout(function() {self.countdown();}, 1000);
    }
  };

};

HatoholReconnectModal.prototype = Object.create(HatoholModal.prototype);
HatoholReconnectModal.prototype.constructor = HatoholReconnectModal;

