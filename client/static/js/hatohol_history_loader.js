/*
 * Copyright (C) 2014 - 2015 Project Hatohol
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

var HatoholHistoryLoader = function(options) {
  var self = this;

  self.options = options;
  self.item = undefined;
  self.servers = undefined;
  self.history = [];
  self.lastQuery = undefined;
  self.loading = false;
};

HatoholHistoryLoader.prototype.load = function() {
  var self = this;
  var deferred = new $.Deferred();

  self.loading = true;

  $.when(loadItem())
    .then(loadHistory)
    .done(onLoadComplete);

  return deferred.promise();

  function onLoadComplete() {
    self.loading = false;
    deferred.resolve();
  }

  function getItemQuery() {
    var options = self.options.query;
    var itemQuery = {
      serverId: options.serverId,
      hostId: options.hostId,
      itemId: options.itemId
    };
    return 'item?' + $.param(itemQuery);
  }

  function getHistoryQuery() {
    var query = $.extend({}, self.options.query);
    var lastData;

    // omit loading existing data
    if (self.history && self.history.length > 0) {
      lastData = self.history[self.history.length - 1];
      query.beginTime = Math.floor(lastData[0] / 1000) + 1;
    }

    // set missing time range
    if (!query.endTime)
      query.endTime = Math.floor(new Date().getTime() / 1000);
    if (!query.beginTime)
      query.beginTime = query.endTime - self.getTimeSpan();

    self.lastQuery = query;

    return 'history?' + $.param(query);
  }

  function loadItem() {
    var deferred = new $.Deferred();
    var view = self.options.view; // TODO: Remove the view dependency

    if (self.item) {
      deferred.resolve();
      return deferred.promise();
    }

    view.startConnection(getItemQuery(), function(reply) {
      var items = reply.items;
      var messageDetail;
      var query = self.options.query;

      if (items && items.length == 1) {
        self.item = items[0];
        self.servers = reply.servers;

        if (self.options.onLoadItem)
          self.options.onLoadItem(self, self.item, self.servers);

        deferred.resolve();
      } else {
        messageDetail =
          "Monitoring Server ID: " + query.serverId + ", " +
          "Host ID: " + query.hostId + ", " +
          "Item ID: " + query.itemId;
        if (!items || items.length === 0)
          hatoholErrorMsgBox(gettext("No such item: ") + messageDetail);
        else if (items.length > 1)
          hatoholErrorMsgBox(gettext("Too many items are found for ") +
                             messageDetail);
        deferred.reject();
      }
    });

    return deferred.promise();
  }

  function loadHistory() {
    var deferred = new $.Deferred();
    var view = self.options.view; // TODO: Remove the view dependency

    view.startConnection(getHistoryQuery(), function(reply) {
      var history = reply.history;

      self.updateHistory(history);

      if (self.options.onLoadHistory)
        self.options.onLoadHistory(self, self.history);

      deferred.resolve();
    });

    return deferred.promise();
  }
};

HatoholHistoryLoader.prototype.updateHistory = function(history) {
  var self = this;

  shiftHistory(self.history);
  appendHistory(self.history, history);

  function appendHistory(history, newHistory) {
    var i, idx = history.length;
    for (i = 0; i < newHistory.length; i++) {
      history[idx++] = [
        // Xaxis: UNIX time in msec
        newHistory[i].clock * 1000 + Math.floor(newHistory[i].ns / 1000000),
        // Yaxis: value
        newHistory[i].value
      ];
    }
    return history;
  }

  function shiftHistory(history) {
    var beginTimeInMSec;

    if (!history)
      return;

    beginTimeInMSec = (self.lastQuery.endTime - self.getTimeSpan()) * 1000;

    while (history.length > 0 && history[0][0] < beginTimeInMSec) {
      if (history[0].length == 1 || history[0][1] > beginTimeInMSec) {
        // remain one point to draw the left edge of the line
        break;
      }
      history.shift();
    }
  }
};

HatoholHistoryLoader.prototype.setTimeRange = function(beginTimeInSec,
                                                       endTimeInSec,
                                                       keepHistory)
{
  if (isNaN(beginTimeInSec))
    delete this.options.query.beginTime;
  else
    this.options.query.beginTime = beginTimeInSec;

  if (isNaN(endTimeInSec))
    delete this.options.query.endTime;
  else
    this.options.query.endTime = endTimeInSec;

  if (!isNaN(beginTimeInSec) && !isNaN(endTimeInSec))
    this.options.defaultTimeSpan = endTimeInSec - beginTimeInSec;

  if (!keepHistory)
    this.history = [];
};

HatoholHistoryLoader.prototype.getTimeSpan = function() {
  var query = this.options.query;
  var secondsInHour = 60 * 60;

  if (!isNaN(query.beginTime) && !isNaN(query.endTime))
    return query.endTime - query.beginTime;

  if (isNaN(this.options.defaultTimeSpan))
    this.options.defaultTimeSpan = secondsInHour * 6;

  return this.options.defaultTimeSpan;
};

HatoholHistoryLoader.prototype.isLoading = function() {
  return this.loading;
};

HatoholHistoryLoader.prototype.getItem = function() {
  return this.item;
};

HatoholHistoryLoader.prototype.getServers = function() {
  return this.servers;
};
