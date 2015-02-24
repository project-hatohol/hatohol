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

var HistoryLoader = function(options) {
  var self = this;

  self.options = options;
  self.item = undefined;
  self.servers = undefined;
  self.history = [];
  self.lastQuery = undefined;
  self.loading = false;
};

HistoryLoader.prototype.load = function() {
  var self = this;
  var deferred = new $.Deferred;

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
    }
    return 'item?' + $.param(itemQuery);
  };

  function getHistoryQuery() {
    var query = $.extend({}, self.options.query);
    var lastReply, lastData;

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
  };

  function loadItem() {
    var deferred = new $.Deferred;
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
        if (!items || items.length == 0)
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
    var deferred = new $.Deferred;
    var view = self.options.view; // TODO: Remove the view dependency

    view.startConnection(getHistoryQuery(), function(reply) {
      var maxRecordsPerRequest = 1000;
      var history = reply.history;

      self.updateHistory(history);

      if (self.options.onLoadHistory)
        self.options.onLoadHistory(self, self.history);

      if (history.length >= maxRecordsPerRequest) {
        $.when(loadHistory()).done(function() {
          deferred.resolve();
        });
      } else {
        deferred.resolve();
      }
    });

    return deferred.promise();
  }
};

HistoryLoader.prototype.updateHistory = function(history) {
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
    var beginTimeInMSec, data;

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
}

HistoryLoader.prototype.setTimeRange = function(beginTimeInSec, endTimeInSec,
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
}

HistoryLoader.prototype.getTimeSpan = function() {
  var query = this.options.query;
  var secondsInHour = 60 * 60;

  if (!isNaN(query.beginTime) && !isNaN(query.endTime))
    return query.endTime - query.beginTime;

  if (isNaN(this.options.defaultTimeSpan))
    this.options.defaultTimeSpan = secondsInHour * 6;

  return this.options.defaultTimeSpan;
}

HistoryLoader.prototype.isLoading = function() {
  return this.loading;
}

HistoryLoader.prototype.getItem = function() {
  return this.item;
}

HistoryLoader.prototype.getServers = function() {
  return this.servers;
}


var HatoholItemSelector = function(options) {
  var self = this;

  options = options || {};
  self.lastIndex = 0;
  self.elementId = 'hatohol-item-list';
  self.servers = options.servers;
  self.lastItemsData = undefined;
  self.rowData = {};
  self.view = options.view; // TODO: Remove view dependency
  self.appendItemCallback = options.appendItemCallback;
  self.removeItemCallback = options.removeItemCallback;

  setup();

  function setup() {
    self.view.setupHostQuerySelectorCallback(
      function() {
        var query = self.view.getHostFilterQuery();
        query.limit = 1; // we need only "servers" object
        self.view.startConnection("items?" + $.param(query), function(reply) {
          self.servers = reply.servers;
          self.setupCandidates();
        });
      },
      '#select-server', '#select-host-group', '#select-host');
    $("#select-item").attr("disabled", "disabled");
    $("#add-item-button").attr("disabled", "disabled");
    $("#select-server").change(loadItemCandidates);
    $("#select-host-group").change(loadItemCandidates);
    $("#select-host").change(loadItemCandidates);
    $("#select-item").change(function() {
      if ($(this).val() == "---------")
        $("#add-item-button").attr("disabled", "disabled");
      else
        $("#add-item-button").removeAttr("disabled");
    });
    $("#add-item-button").click(function() {
      var query = self.view.getHostFilterQuery();
      var i, index, item;

      query.itemId = $("#select-item").val();

      for (i = 0; i < self.lastItemsData.items.length; i++) {
        if (self.lastItemsData.items[i].id == query.itemId)
          item = self.lastItemsData.items[i];
      }

      index = self.appendItem(item, self.lastItemsData.servers,
                              query.hostgroupId);
      setItemCandidates(self.lastItemsData);

      if (self.appendItemCallback)
        self.appendItemCallback(index, query);
    });
  }

  function isAlreadyAddedItem(item1) {
    var i, item2;
    for (i in self.rowData) {
      item2 = self.rowData[i].item;
      if (!item2)
        continue;
      if (item1.serverId == item2.serverId &&
          item1.hostId == item2.hostId &&
          item1.id == item2.id)
        return true;
    }
    return false;
  }

  function setItemCandidates(reply) {
    var candidates = $.map(reply.items, function(item) {
      if (isAlreadyAddedItem(item))
        return null;
      var label = getItemBriefWithUnit(item);
      return { label: label, value: item.id };
    });
    self.view.setFilterCandidates($("#select-item"), candidates);
    self.lastItemsData = reply;
  }

  function loadItemCandidates() {
    var query;
    var hostName = $("#select-host").val();

    $("#add-item-button").attr("disabled", "disabled");

    if (hostName == "---------") {
      self.view.setFilterCandidates($("#select-item"));
      return;
    }

    query = self.view.getHostFilterQuery();
    self.view.startConnection("items?" + $.param(query), setItemCandidates);
  }
}

HatoholItemSelector.prototype.show = function() {
  var self = this;
  if (!self.servers) {
    self.view.startConnection("item?limit=1", function(reply) {
      self.servers = reply.servers;
      self.setupCandidates();
    });
  }
  $('#' + self.elementId).modal('show');
}

HatoholItemSelector.prototype.hide = function() {
  $('#' + this.elementId).modal('hide');
}

HatoholItemSelector.prototype.appendItem = function(item, servers, hostgroupId)
{
  return this.setItem(undefined, item, servers, hostgroupId);
}

HatoholItemSelector.prototype.setItem = function(index, item, servers,
                                                 hostgroupId)
{
  var self = this;
  var server = item ? servers[item.serverId] : undefined;
  var serverName = item ? getNickName(server, item.serverId) : "-";
  var hostName = item ? getHostName(server, item.hostId) : "-";
  var groupName = (hostgroupId && hostgroupId != -1) ?
    getHostgroupName(server, hostgroupId) : "-";
  var itemName = item ? getItemBriefWithUnit(item)  : "-";
  var id, tr;

  if (isNaN(index))
    index = self.lastIndex++;
  id = self.elementId + "-row-" + index;

  if (index in self.rowData) {
    tr = $("#" + id);
    tr.empty();
  } else {
    tr = $("<tr>", { id: id });
  }

  tr.append($("<td>"));
  tr.append($("<td>", { text: serverName }));
  tr.append($("<td>", { text: groupName }));
  tr.append($("<td>", { text: hostName }));
  tr.append($("<td>", { text: itemName }));
  tr.append($("<td>").append(
    $("<button>", {
      text: gettext("DELETE"),
      type: "button",
      class: "btn btn-default",
      click: function() {
        var index = $(this).attr("itemIndex");
        $(this).parent().parent().remove();
        if (self.removeItemCallback)
          self.removeItemCallback(index);
        delete self.rowData[index];
      }
    }).attr("itemIndex", index)));

  if (!(index in self.rowData)) {
    self.rowData[index] = {};
    tr.insertBefore("#" + self.elementId + " tbody tr :last");
  }

  if (item) {
    self.rowData[index] = self.rowData[index] || {};
    self.rowData[index].item = item;
    self.rowData[index].hostgroupId = hostgroupId;
  }

  return index;
}

HatoholItemSelector.prototype.setUserData = function(index, data) {
  this.rowData[index] = this.rowData[index] || {};
  this.rowData[index].userData = data;
}

HatoholItemSelector.prototype.getUserData = function(index) {
  if (this.rowData[index])
    return this.rowData[index].userData;
  else
    return undefined;
}

HatoholItemSelector.prototype.getIndexByUserData = function(data) {
  for (index in this.rowData) {
    if (this.rowData[index] && this.rowData[index].userData == data)
      return parseInt(index);
  }
  return -1;
}

HatoholItemSelector.prototype.setupCandidates = function() {
  this.view.setServerFilterCandidates(this.servers);
  this.view.setHostgroupFilterCandidates(this.servers);
  this.view.setHostFilterCandidates(this.servers);
}

HatoholItemSelector.prototype.setServers = function(servers) {
  if (!this.servers)
    this.servers = servers;
  this.view.setupHostFilters(servers);
}

HatoholItemSelector.prototype.getConfig = function() {
  var i, item, data, config = { histories: [] };
  for (i in this.rowData) {
    item = this.rowData[i].item;
    data = {
      serverId: item.serverId,
      hostId: item.hostId,
      hostgroupId: this.rowData[i].hostgroupId,
      itemId: item.id,
    };
    config.histories.push(data);
  }
  return config;
}
