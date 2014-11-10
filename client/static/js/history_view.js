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

  load();

  function updateView(reply) {
    replyHistory = reply;
    self.displayUpdateTime();
  }

  function getItemQuery() {
    return 'item?' + $.param(query);
  };

  function getHistoryQuery() {
    return 'history?' + $.param(query);
  };

  function onItemLoad(reply) {
    replyItem = reply;
    self.startConnection(getHistoryQuery(), updateView);
  }

  function load() {
    self.startConnection(getItemQuery(), onItemLoad);
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
