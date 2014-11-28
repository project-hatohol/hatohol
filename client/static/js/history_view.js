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
  var itemQuery, historyQuery;
  var secondsInHour = 60 * 60;

  self.reloadIntervalSeconds = 60;
  self.replyItem = null;
  self.lastQuery = null;
  self.timeSpan = null;
  self.plotData = null;
  self.plotOptions = null;
  self.plot = null;
  self.sliderTimeRange = null;

  if (!options)
    options = {};

  itemQuery = self.parseItemQuery(options.query);
  historyQuery = self.parseHistoryQuery(options.query);

  appendGraphArea();
  loadItemAndHistory();

  function appendGraphArea() {
    $("#main").append($("<div>", {
      id: "item-graph",
      height: "300px",
    }));
    $("#main").append($("<div>", {
      id: "item-graph-slider",
    }));

    $("#item-graph").bind("plotselected", function (event, ranges) {
      var options;

      // clamp the zooming to prevent eternal zoom
      if (ranges.xaxis.to - ranges.xaxis.from < 60 * 1000) {
        ranges.xaxis.from -= 30 * 1000;
        ranges.xaxis.to = ranges.xaxis.from + 60 * 1000;
      }

      // zoom
      options = $.extend(true, {}, self.plotOptions, {
        xaxis: { min: ranges.xaxis.from, max: ranges.xaxis.to },
        yaxis: { min: ranges.yaxis.from, max: ranges.yaxis.to }
      });
      $.plot("#item-graph", self.plotData, options);

      setSliderTimeRange(Math.floor(ranges.xaxis.from / 1000),
                         Math.floor(ranges.xaxis.to / 1000));
    });

    // zoom cancel
    $("#item-graph").bind("dblclick", function (event) {
      $.plot("#item-graph", self.plotData, self.plotOptions);
      setSliderTimeRange(Math.floor(self.plotOptions.xaxis.min / 1000),
                         Math.floor(self.plotOptions.xaxis.max / 1000));
    });
  };

  function formatPlotData(item, historyReplies) {
    var i;
    var data = [ { label: item.brief, data:[] } ];

    if (item.unit)
      data[0].label += " [" + item.unit + "]";

    if (!historyReplies)
      return data;

    for (i = 0; i < historyReplies.length; i++)
      appendPlotData(data, historyReplies[i]);

    return data;
  };

  function appendPlotData(plotData, historyReply) {
    var i, idx = plotData[0].data.length;
    var history = historyReply.history;
    for (i = 0; i < history.length; i++) {
      plotData[0].data[idx++] = [
        // Xaxis: UNIX time in msec
        history[i].clock * 1000 + Math.floor(history[i].ns / 1000000),
        // Yaxis: value
        history[i].value
      ];
    }
    return plotData;
  }

  function timeTickFormatter(val, axis) {
    var now = new Date();
    var date = $.plot.dateGenerator(val, self.plotOptions.xaxis);
    var unit = axis.tickSize[1];
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

  function drawGraph(item, history) {
    var beginTimeInSec = self.lastQuery.endTime - self.timeSpan;
    var endTimeInSec = self.lastQuery.endTime;
    var options = {
      xaxis: {
        mode: "time",
        timezone: "browser",
        tickFormatter: timeTickFormatter,
        min: beginTimeInSec * 1000,
        max: endTimeInSec * 1000,
      },
      yaxis: {
        min: 0,
        tickFormatter: function(val, axis) {
          return formatItemValue("" + val, item.unit);
        }
      },
      legend: {
        show: false,
        position: "sw",
      },
      points: {
      },
      selection: {
        mode: "x",
      },
    };
    if (item.valueType == hatohol.ITEM_INFO_VALUE_TYPE_INTEGER)
      options.yaxis.minTickSize = 1;
    if (history[0].data.length < 3)
      options.points.show = true;

    self.plotOptions = options;
    self.plot = $.plot($("#item-graph"), history, self.plotOptions);
  }

  function drawSlider() {
    var beginTimeInSec = self.lastQuery.endTime - self.timeSpan;
    var endTimeInSec = self.lastQuery.endTime;
    if (!self.sliderTimeRange || !historyQuery.endTime) {
      self.sliderTimeRange = {
        min: self.lastQuery.endTime - secondsInHour * 24 * 7,
        max: self.lastQuery.endTime
      }
    }
    var timeRange = {
      last: [beginTimeInSec, endTimeInSec],
      minSpan: secondsInHour,
      maxSpan: secondsInHour * 24,
      min: self.sliderTimeRange.min,
      max: self.sliderTimeRange.max,
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

    $("#item-graph-slider").slider({
      range: true,
      min: timeRange.min,
      max: timeRange.max,
      values: timeRange.last,
      change: function(event, ui) {
        if (self.settingSliderTimeRange)
          return;
        if (self.loadingHistory)
          return;

        timeRange.set(ui.values);
        setSliderTimeRange(timeRange.last[0], timeRange.last[1]);
        setGraphTimeRange(timeRange.last[0], timeRange.last[1]);

        self.plotData = null;
        historyQuery.beginTime = timeRange.last[0];
        historyQuery.endTime = timeRange.last[1];
        self.timeSpan = timeRange.last[1] - timeRange.last[0];
        loadHistory();
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

  function updateView(reply) {
    var item = self.replyItem.items[0];
    if (!self.plotData)
      self.plotData = formatPlotData(item);
    appendPlotData(self.plotData, reply);
    self.displayUpdateTime();
    drawGraph(item, self.plotData);
    drawSlider();
  }

  function getItemQuery() {
    return 'item?' + $.param(itemQuery);
  };

  function getHistoryQuery() {
    var query = $.extend({}, historyQuery);
    var defaultTimeSpan = secondsInHour * 6;
    var lastReply, lastData, now;

    // omit loading existing data
    if (self.plotData && self.plotData[0].data.length > 0) {
      lastData = self.plotData[0].data[self.plotData[0].data.length - 1];
      query.beginTime = Math.floor(lastData[0] / 1000) + 1;
    }

    // set missing time range
    if (!query.endTime) {
      now = new Date();
      query.endTime = Math.floor(now.getTime() / 1000);
    }
    if (!query.beginTime)
      query.beginTime = query.endTime - defaultTimeSpan;

    if (!self.timeSpan)
      self.timeSpan = query.endTime - query.beginTime;
    self.lastQuery = query;

    return 'history?' + $.param(query);
  };

  function buildHostName(itemReply) {
    var item = itemReply.items[0];
    var server = itemReply.servers[item.serverId];
    var serverName = getServerName(server, item["serverId"]);
    var hostName   = getHostName(server, item["hostId"]);
    return serverName + ": " + hostName;
  }

  function setItemDescription(itemReply) {
    var item = itemReply.items[0];
    var hostName = buildHostName(itemReply);
    var title = "";
    title += item.brief;
    title += " (" + hostName + ")";
    $("title").text(title);
    $(".graph h2").text(title);
  }

  function onLoadItem(reply) {
    var items = reply["items"];
    var messageDetail;

    self.replyItem = reply;

    if (items && items.length == 1) {
      setItemDescription(reply);
      loadHistory();
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

  function onLoadHistory(reply) {
    var maxRecordsPerRequest = 1000;
    updateView(reply);
    if (reply.history.length >= maxRecordsPerRequest) {
      loadHistory();
    } else {
      if (!historyQuery.endTime)
        self.setAutoReload(loadHistory, self.reloadIntervalSeconds);
      self.loadingHistory = false;
    }
  }

  function loadHistory() {
    self.clearAutoReload();
    self.loadingHistory = true;
    self.startConnection(getHistoryQuery(), onLoadHistory);
  }

  function loadItemAndHistory() {
    self.startConnection(getItemQuery(), onLoadItem);
  }
};

HistoryView.prototype = Object.create(HatoholMonitoringView.prototype);
HistoryView.prototype.constructor = HistoryView;

HistoryView.prototype.parseQuery = function(query, knownKeys) {
  var i, allParams = deparam(query), queryTable = {};
  for (i = 0; i < knownKeys.length; i++) {
    if (knownKeys[i] in allParams)
      queryTable[knownKeys[i]] = allParams[knownKeys[i]];
  }
  return queryTable;
};

HistoryView.prototype.parseItemQuery = function(query) {
  var knownKeys = ["serverId", "hostId", "itemId"];
  return this.parseQuery(query, knownKeys);
};

HistoryView.prototype.parseHistoryQuery = function(query) {
  var knownKeys = ["serverId", "hostId", "itemId", "beginTime", "endTime"];
  var startTime = new Date();
  queryTable = this.parseQuery(query, knownKeys);
  return queryTable;
};

HistoryView.prototype.showError = function(message) {
  hatoholErrorMsgBox(message);
};
