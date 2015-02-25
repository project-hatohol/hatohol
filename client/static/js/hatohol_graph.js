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


var HatoholGraph = function(options) {
  var self = this;

  options = options || {};

  self.id = options.id || "hatohol-graph";
  self.plotData = [];
  self.plotOptions = undefined;
  self.plot = undefined;
  self.loaders = [];
  self.title = "";

  setup();

  function setup() {
    $("#" + self.id).bind("plotselected", function (event, ranges) {
      var plotOptions;

      // clamp the zooming to prevent eternal zoom
      if (ranges.xaxis.to - ranges.xaxis.from < 60 * 1000) {
        ranges.xaxis.from -= 30 * 1000;
        ranges.xaxis.to = ranges.xaxis.from + 60 * 1000;
      }

      // zoom
      plotOptions = $.extend(true, {}, self.plotOptions, {
        xaxis: { min: ranges.xaxis.from, max: ranges.xaxis.to },
        yaxis: { min: ranges.yaxis.from, max: ranges.yaxis.to }
      });
      $.plot("#" + self.id, self.plotData, plotOptions);

      if (options.zoomCallback)
        options.zoomCallback(Math.floor(ranges.xaxis.from / 1000),
                             Math.floor(ranges.yaxis.to / 1000));
    });

    // zoom cancel
    $("#" + self.id).bind("dblclick", function (event) {
      $.plot("#" + self.id, self.plotData, self.plotOptions);

      if (options.zoomCallback)
        options.zoomCallback(Math.floor(self.plotOptions.xaxis.min / 1000),
                             Math.floor(self.plotOptions.xaxis.max / 1000));
    });
  }
}

HatoholGraph.prototype.draw = function(beginSec, endSec) {
  var self = this;
  var i;

  self._updatePlotData();
  self._updateTitleAndLegendLabels();
  self.plotOptions = getPlotOptions(beginSec, endSec);
  fixupYAxisMapping(self.plotData, self.plotOptions);

  for (i = 0; i < self.plotData.length; i++) {
    if (self.plotData[i].data.length == 1)
      self.plotOptions.points.show = true;
  }

  self.plot = $.plot($("#" + self.id), self.plotData, self.plotOptions);


  function getYAxisOptions(unit) {
    return {
      min: 0,
      unit: unit,
      tickFormatter: function(val, axis) {
        return formatItemValue("" + val, this.unit);
      }
    };
  }

  function getYAxesOptions() {
    var i, item, label, axis, axes = [], table = {}, isInt;
    for (i = 0; i < self.loaders.length; i++) {
      item = self.loaders[i].getItem();
      label = item ? item.unit : "";
      axis = item ? table[item.unit] : undefined;
      isInt = item && (item.valueType == hatohol.ITEM_INFO_VALUE_TYPE_INTEGER);
      if (axis) {
        if (!isInt)
          delete axis.minTickSize;
      } else {
        axis = getYAxisOptions(label);
        if (isInt)
          axis.minTickSize = 1;
        if (axes.length % 2 == 1)
          axis.position = "right";
        axes.push(axis);
        table[label] = axis;
      }
    }
    return axes;
  }

  function getDate(timeMSec) {
    var plotOptions = {
      mode: "time",
      timezone: "browser"
    };
    return $.plot.dateGenerator(timeMSec, plotOptions);
  }

  function formatTime(val, unit) {
    var now = new Date();
    var date = getDate(val);
    var format = "";

    if (now.getFullYear() != date.getFullYear())
      format += "%Y/";
    if (now.getFullYear() != date.getFullYear() ||
        now.getMonth() != date.getMonth() ||
        now.getDate() != date.getDate()) {
      format += "%m/%d ";
    }
    if (unit == "hour" || unit == "minute" || unit == "second")
      format += "%H:%M";
    if (unit == "second")
      format += ":%S";

    return $.plot.formatDate(date, format);
  }

  function getPlotOptions(beginTimeInSec, endTimeInSec) {
    return {
      xaxis: {
        mode: "time",
        timezone: "browser",
        tickFormatter: function(val, axis) {
          var unit = axis.tickSize[1];
          return formatTime(val, unit);
        },
        min: beginTimeInSec * 1000,
        max: endTimeInSec * 1000,
      },
      yaxes: getYAxesOptions(),
      legend: {
        show: (self.plotData.length > 1),
        position: "sw",
      },
      points: {
      },
      selection: {
        mode: "x",
      },
    };
  }

  function fixupYAxisMapping(plotData, plotOptions) {
    function findYAxis(yaxes, item) {
      var i;
      for (i = 0; item && i < yaxes.length; i++) {
        if (yaxes[i].unit == item.unit)
          return i + 1;
      }
      return 1;
    }
    var i, item;

    for (i = 0; i < self.loaders.length; i++) {
      item = self.loaders[i].getItem();
      plotData[i].yaxis = findYAxis(plotOptions.yaxes, item);
    }
  }
}

HatoholGraph.prototype._createLegendData = function(item, servers) {
  var legend = { data:[] };

  if (item) {
    // it will be filled by updateTitleAndLegendLabels()
    legend.label = undefind;
    if (item.unit)
      legend.label += " [" + item.unit + "]";
  }

  return legend;
}

HatoholGraph.prototype.appendHistoryLoader = function(loader) {
  var self = this;

  self.loaders.push(loader);
  self.plotData.push(self._createLegendData(loader.getItem(),
                                            loader.getServers()));
}

HatoholGraph.prototype.removeHistoryLoader = function(loader) {
  var self = this;
  var i;
  for (i = 0; i < self.loaders.length; i++) {
    if (self.loaders[i] == loader) {
      self.loaders.splice(i, 1);
      self.plotData.splice(i, 1);
      return;
    }
  }
}

HatoholGraph.prototype._updatePlotData = function() {
  var self = this;
  var i, item, servers;
  for (i = 0; i < self.loaders.length; i++) {
    if (self.plotData[i].data) {
      self.plotData[i].data = self.loaders[i].history;
    } else {
      item = self.loaders[i].getItem();
      servers = self.loaders[i].getServers();
      self.plotData[i] = self._createLegendData(item, servers);
    }
  }
}

HatoholGraph.prototype._updateTitleAndLegendLabels = function() {
  var self = this;

  doUpdate();

  function buildHostName(item, servers) {
    if (!item || !servers)
      return gettext("Unknown host")
    var server = servers[item.serverId];
    var serverName = getServerName(server, item["serverId"]);
    var hostName   = getHostName(server, item["hostId"]);
    return serverName + ": " + hostName;
  }

  function buildTitle(item, servers) {
    var hostName, title = "";

    if (!item || !servers)
      return gettext("History");
    hostName = buildHostName(item, servers);

    title += item.brief;
    title += " (" + hostName + ")";
    if (item.unit)
      title += " [" + item.unit + "]";
    return title;
  }

  function isSameHost() {
    var i, item, prevItem = self.loaders[0].getItem();

    if (!prevItem)
      return false;

    for (i = 1; i < self.loaders.length; i++) {
      if (!self.loaders[i])
        return false;

      item = self.loaders[i].getItem();
      if (!item)
        return false;
      if (item.serverId != prevItem.serverId)
        return false;
      if (item.hostId != prevItem.hostId)
        return false;

      prevItem = item;
    }

    return true;
  }

  function isSameItem() {
    var i, item, prevItem = self.loaders[0].getItem();

    if (!prevItem)
      return false;

    for (i = 1; i < self.loaders.length; i++) {
      if (!self.loaders[i])
        return false;

      item = self.loaders[i].getItem();
      if (!item)
        return false;
      if (item.brief != prevItem.brief)
        return false;

      prevItem = item;
    }

    return true;
  }

  function doUpdate() {
    var i, item, servers, loader = self.loaders[0];

    self.title = "";

    if (self.plotData.length == 0) {
      self.title = undefined;
    } else if (self.plotData.length == 1) {
      self.title = buildTitle(loader.getItem(), loader.getServers());
    } else if (isSameHost()) {
      self.title = buildHostName(loader.getItem(), loader.getServers());
      for (i = 0; i < self.plotData.length; i++) {
        // omit host names in legend labels
        item = self.loaders[i].getItem();
        self.plotData[i].label = getItemBriefWithUnit(item);
      }
    } else if (isSameItem()) {
      self.title = getItemBriefWithUnit(loader.getItem());
      for (i = 0; i < self.plotData.length; i++) {
        // omit item names in legend labels
        item = self.loaders[i].getItem();
        servers = self.loaders[i].getServers();
        self.plotData[i].label = buildHostName(item, servers);
      }
    } else {
      for (i = 0; i < self.plotData.length; i++) {
        item = self.loaders[i].getItem();
        servers = self.loaders[i].getServers();
        self.plotData[i].label = buildTitle(item, servers);
      }
    }
  }
}
