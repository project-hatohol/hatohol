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

      if (items && items.length == 1) {
        self.item = items[0];
        self.servers = reply.servers;

        if (self.options.onLoadItem)
          self.options.onLoadItem(self.item, self.servers);

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
        self.options.onLoadHistory(self.history);

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

HistoryLoader.prototype.setTimeRange = function(beginTimeInSec, endTimeInSec, keepHistory) {
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


var HistoryView = function(userProfile, options) {
  var self = this;
  var secondsInHour = 60 * 60;
  var i = 0;
  var historyQueries;

  options = options || {};

  self.reloadIntervalSeconds = 60;
  self.autoReloadIsEnabled = false;
  self.plotData = [];
  self.plotOptions = undefined;
  self.plot = undefined;
  self.timeRange = undefined;
  self.settingSliderTimeRange = false;
  self.endTime = undefined;
  self.timeSpan = secondsInHour * 6;
  self.loaders = [];

  appendGraphArea();

  historyQueries = self.parseQuery(options.query);
  for (i = 0; i < historyQueries.length; i++) {
    self.plotData[i] = initLegendData();
    self.loaders[i] = new HistoryLoader({
      index: i,
      view: this,
      defaultTimeSpan: self.timeSpan,
      query: historyQueries[i],
      onLoadItem: function(item, servers) {
	this.item = item;
        self.plotData[this.index] = initLegendData(item, servers);
        updateView(this.item);
      },
      onLoadHistory: function(history) {
        self.plotData[this.index].data = history;
        updateView(this.item);
      }
    });
  }

  // TODO: allow different time ranges?
  self.endTime = self.loaders[0].options.query.endTime;
  self.timeSpan = self.loaders[0].getTimeSpan();
  self.autoReloadIsEnabled = !self.endTime;

  load();

  function load() {
    var promises;
    var i;

    self.clearAutoReload();
    if (self.autoReloadIsEnabled) {
      self.endTime = Math.floor(new Date().getTime() / 1000);
      for (i = 0; i < self.loaders.length; i++)
        self.loaders[i].setTimeRange(undefined, self.endTime, true);
    }

    promises = $.map(self.loaders, function(loader) { return loader.load(); });
    $.when.apply($, promises).done(function() {
      if (self.autoReloadIsEnabled) {
        self.setAutoReload(load, self.reloadIntervalSeconds);
      } else {
        disableAutoRefresh();
      }
    });
  }

  function enableAutoRefresh(onClickButton) {
    var button = $("#item-graph-auto-refresh");

    button.removeClass("btn-default");
    button.addClass("btn-primary");
    if (!onClickButton)
      button.addClass("active");

    self.autoReloadIsEnabled = true;
    load();
  }

  function disableAutoRefresh(onClickButton) {
    var button = $("#item-graph-auto-refresh");
    self.clearAutoReload();
    self.autoReloadIsEnabled = false;
    if (!onClickButton)
      button.removeClass("active");
    button.removeClass("btn-primary");
    button.addClass("btn-default");
  }

  function appendGraphArea() {
    $("#main").append($("<div>", {
      id: "item-graph",
      height: "300px",
    }))
    .append($("<div>", {
      id: "item-graph-slider-area",
    }));

    $("#item-graph-slider-area").append($("<div>", {
      id: "item-graph-slider",
    }))
    .append($("<button>", {
      id: "item-graph-auto-refresh",
      type: "button",
      class: "btn btn-primary glyphicon glyphicon-refresh active",
    }).attr("data-toggle", "button"));

    $("#item-graph").bind("plotselected", function (event, ranges) {
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
      $.plot("#item-graph", self.plotData, plotOptions);

      setSliderTimeRange(Math.floor(ranges.xaxis.from / 1000),
                         Math.floor(ranges.xaxis.to / 1000));

      // disable auto refresh
      disableAutoRefresh();
    });

    // zoom cancel
    $("#item-graph").bind("dblclick", function (event) {
      $.plot("#item-graph", self.plotData, self.plotOptions);
      setSliderTimeRange(Math.floor(self.plotOptions.xaxis.min / 1000),
                         Math.floor(self.plotOptions.xaxis.max / 1000));
    });

    // toggle auto refresh
    $("#item-graph-auto-refresh").on("click", function() {
      if ($(this).hasClass("active")) {
        disableAutoRefresh(true);
      } else {
        enableAutoRefresh(true);
      }
    });
  };

  function initLegendData(item, servers) {
    var legend = { data:[] };

    if (item) {
      legend.label = buildTitle(item, servers);
      if (item.unit)
	legend.label += " [" + item.unit + "]";
    }

    return legend;
  };

  function formatTime(val, unit) {
    var now = new Date();
    var date = $.plot.dateGenerator(val, self.plotOptions.xaxis);
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

  function createYAxisOptions(unit) {
    var options = {
      min: 0,
      unit: unit,
      tickFormatter: function(val, axis) {
        return formatItemValue("" + val, this.unit);
      }
    };
    return options;
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
        axis = createYAxisOptions(label);
        if (isInt)
          options.minTickSize = 1;
        axes.push(axis);
        table[label] = axis;
      }
    }
    return axes;
  }

  function getPlotOptions(beginTimeInSec, endTimeInSec) {
    var plotOptions = {
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

    return plotOptions;
  }

  function drawGraph(plotData) {
    var beginTimeInSec = self.endTime - self.timeSpan;
    var endTimeInSec = self.endTime;
    var plotOptions = getPlotOptions(beginTimeInSec, endTimeInSec);
    var i;

    for (i = 0; i < plotData.length; i++) {
      if (plotData[i].data.length == 1)
        plotOptions.points.show = true;
    }

    self.plotOptions = plotOptions;
    self.plot = $.plot($("#item-graph"), plotData, plotOptions);
  }

  function getTimeRange() {
    var beginTimeInSec, endTimeInSec, date, min;

    if (self.timeRange && !self.autoReloadIsEnabled)
      return self.timeRange;

    beginTimeInSec = self.endTime - self.timeSpan;
    endTimeInSec = self.endTime;

    // Adjust to 00:00:00
    min = self.endTime - secondsInHour * 24 * 7;
    date = $.plot.dateGenerator(min * 1000, self.plotOptions.xaxis);
    min -= date.getHours() * 3600 + date.getMinutes() * 60 + date.getSeconds();

    self.timeRange = {
      last: [beginTimeInSec, endTimeInSec],
      minSpan: secondsInHour,
      maxSpan: secondsInHour * 24,
      min: min,
      max: self.endTime,
      set: function(range) {
        this.last = range.slice();
        if (this.last[1] - this.last[0] > this.maxSpan)
          this.last[0] = this.last[1] - this.maxSpan;
        if (this.last[1] - this.last[0] < this.minSpan) {
          if (this.last[0] + this.minSpan >= this.max) {
            this.last[1] = this.max;
            this.last[0] = this.max - this.minSpan;
          } else {
            this.last[1] = this.last[0] + this.minSpan;
          }
        }
      },
      get: function() {
        return this.last;
      },
      getSpan: function() {
        return this.last[1] - this.last[0];
      },
    }

    return self.timeRange;
  }

  function drawSlider() {
    var timeRange = getTimeRange();

    $("#item-graph-slider").slider({
      range: true,
      min: timeRange.min,
      max: timeRange.max,
      values: timeRange.last,
      change: function(event, ui) {
        var i;

        if (self.settingSliderTimeRange)
          return;
        if (self.isLoading())
          return;

        timeRange.set(ui.values);
        setSliderTimeRange(timeRange.last[0], timeRange.last[1]);
        setGraphTimeRange(timeRange.last[0], timeRange.last[1]);
        for (i = 0; i < self.loaders.length; i++)
          self.loaders[i].setTimeRange(timeRange.last[0], timeRange.last[1]);
        self.endTime = timeRange.last[1];
        self.timeSpan = timeRange.last[1] - timeRange.last[0];
        self.autoReloadIsEnabled = false;
        load();
        $("#item-graph-auto-refresh").removeClass("active");
      },
      slide: function(event, ui) {
        var beginTime = ui.values[0], endTime = ui.values[1];

        if (timeRange.last[0] != ui.values[0])
          endTime = ui.values[0] + timeRange.getSpan();
        if (ui.values[1] - ui.values[0] < timeRange.minSpan)
          beginTime = ui.values[1] - timeRange.minSpan;
        timeRange.set([beginTime, endTime]);
        setSliderTimeRange(timeRange.last[0], timeRange.last[1]);
      },
    });
    $("#item-graph-slider").slider('pips', {
      rest: 'label',
      last: false,
      step: secondsInHour * 12,
      formatLabel: function(val) {
        var now = new Date();
        var date = new Date(val * 1000);
        var dayLabel = {
          0: gettext("Sun"),
          1: gettext("Mon"),
          2: gettext("Tue"),
          3: gettext("Wed"),
          4: gettext("Thu"),
          5: gettext("Fri"),
          6: gettext("Sat"),
        }
        if (date.getHours() == 0) {
          if (now.getTime() - date.getTime() > secondsInHour * 24 * 7 * 1000)
            return formatDate(val);
          else
            return dayLabel[date.getDay()];
        } else {
          return "";
        }
      },
    });
    $("#item-graph-slider").slider('float', {
      formatLabel: function(val) {
        return formatDate(val);
      },
    });
  }

  function setSliderTimeRange(min, max) {
    var values;

    if (self.settingSliderTimeRange)
      return;

    self.settingSliderTimeRange = true;
    values = $("#item-graph-slider").slider("values");
    if (min != values[0])
      $("#item-graph-slider").slider("values", 0, min);
    if (max != values[1])
      $("#item-graph-slider").slider("values", 1, max);
    self.settingSliderTimeRange = false;
  }

  function setGraphTimeRange(min, max) {
    $.each(self.plot.getXAxes(), function(index, axis) {
      axis.options.min = min * 1000;
      axis.options.max = max * 1000;
    });
    self.plot.setupGrid();
    self.plot.draw();
  }

  function updateView(item) {
    updateTitleAndLegendLabels();
    self.displayUpdateTime();
    drawGraph(self.plotData);
    drawSlider();
  }

  function buildHostName(item, servers) {
    var server = servers[item.serverId];
    var serverName = getServerName(server, item["serverId"]);
    var hostName   = getHostName(server, item["hostId"]);
    return serverName + ": " + hostName;
  }

  function buildTitle(item, servers) {
    var hostName = buildHostName(item, servers);
    var title = "";
    title += item.brief;
    title += " (" + hostName + ")";
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

  function updateTitleAndLegendLabels() {
    var i, title, item, servers, loader = self.loaders[0];

    if (self.plotData.length == 1) {
      title = buildTitle(loader.getItem(), loader.getServers());
    } if (isSameHost()) {
      title = buildHostName(loader.getItem(), loader.getServers());
      for (i = 0; i < self.plotData.length; i++) {
        // omit host names in legend labels
        item = self.loaders[i].getItem();
        self.plotData[i].label = item.brief;
        if (item.unit)
          self.plotData[i].label += " [" + item.unit + "]";
      }
    } else if (isSameItem()) {
      title = loader.getItem().brief;
      for (i = 0; i < self.plotData.length; i++) {
        // omit item names in legend labels
        item = self.loaders[i].getItem();
        servers = self.loaders[i].getServers();
        self.plotData[i].label = buildHostName(item, servers);
      }
    } else {
      title = gettext("History");
    }

    $("title").text(title);
    $(".graph h2").text(title);
  }
};

HistoryView.prototype = Object.create(HatoholMonitoringView.prototype);
HistoryView.prototype.constructor = HistoryView;

HistoryView.prototype.parseQuery = function(query) {
  var allParams = deparam(query);
  var histories = allParams["histories"]
  var i, tables = [];

  var addHistoryQuery = function(params) {
    var knownKeys = ["serverId", "hostId", "itemId", "beginTime", "endTime"];
    var i, table = {};

    for (i = 0; i < knownKeys.length; i++) {
      if (knownKeys[i] in params)
        table[knownKeys[i]] = params[knownKeys[i]];
    }
    if (Object.keys(table).length > 0)
      tables.push(table);
  }

  addHistoryQuery(allParams);
  for (i = 0; histories && i < histories.length; i++)
    addHistoryQuery(histories[i]);

  return tables;
};

HistoryView.prototype.isLoading = function() {
  var i;
  for (i = 0; i < this.loaders.length; i++)
    if (this.loaders[i].isLoading())
      return true;
  return false;
};
