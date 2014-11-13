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

var HistoryView = function(userProfile, options) {
  var self = this;
  var replyItem, replyHistory;
  var query;

  if (!options)
    options = {};
  query = self.parseQuery(options.query);

  appendGraphArea();
  load();

  function appendGraphArea() {
    $("#main").append($("<div>", {
      id: "item-graph",
      width: "800px",
      height: "300px",
    }));
  };

  function formatHistoryData() {
    var history = replyHistory["history"];
    var i, data = [[]];
    for (i = 0; i < history.length; i++) {
      data[0][i] = [
        // Xaxis: UNIX time in msec
        history[i].clock * 1000 + Math.floor(history[i].ns / 1000000),
        // Yaxis: value
        history[i].value
      ];
    }
    return data;
  };

  function updateView(reply) {
    replyHistory = reply;
    self.displayUpdateTime();
    var options = {
      xaxis: { mode: "time" }
    };
    $.plot($("#item-graph"), formatHistoryData(), options);
  }

  function getItemQuery() {
    return 'item?' + $.param(query);
  };

  function getHistoryQuery() {
    return 'history?' + $.param(query);
  };

  function onLoadItem(reply) {
    var items = reply["items"];
    var messageDetail;

    replyItem = reply;

    if (items && items.length == 1) {
      self.startConnection(getHistoryQuery(), updateView);
    } else {
      messageDetail =
        "Monitoring Server ID: " + query.serverId + ", " +
        "Host ID: " + query.hostId + ", " +
        "Item ID: " + query.itemId;
      if (!items || items.length == 0)
        self.showError(gettext("No such item: ") + messageDetail);
      else if (items.length > 1)
        self.showError(gettext("Too many items are found for ") +
                       messageDetail);
    }
  }

  function load() {
    self.startConnection(getItemQuery(), onLoadItem);
  }
};

HistoryView.prototype = Object.create(HatoholMonitoringView.prototype);
HistoryView.prototype.constructor = HistoryView;

HistoryView.prototype.parseQuery = function(query) {
  var knownKeys = ["serverId", "hostId", "itemId"];
  var i, allParams = deparam(query), queryTable = {};
  for (i = 0; i < knownKeys.length; i++) {
    if (knownKeys[i] in allParams)
      queryTable[knownKeys[i]] = allParams[knownKeys[i]];
  }
  return queryTable;
};

HistoryView.prototype.showError = function(message) {
  hatoholErrorMsgBox(message);
};
