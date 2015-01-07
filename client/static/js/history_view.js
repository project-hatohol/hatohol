/*
 * Copyright (C) 2014 Project Hatohol
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

var HistoryView = function(userProfile, options) {
  var self = this;
  var itemQuery, historyQuery;
  var secondsInHour = 60 * 60;

  self.reloadIntervalSeconds = 60;
  self.item = null;
  self.servers = null;
  self.lastQuery = null;
  self.timeSpan = null;
  self.plotData = null;
  self.plotOptions = null;
  self.plot = null;
  self.timeRange = null;
  self.loadingHistory = false;
  self.settingSliderTimeRange = false;

  if (!options)
    options = {};

  itemQuery = self.parseItemQuery(options.query);
  historyQuery = self.parseHistoryQuery(options.query);

  appendGraphArea();
  $.when(loadItem())
    .then(loadHistory)
    .done(onLoadAllHistory);

  function enableAutoRefresh(onClickButton) {
    var button = $("#item-graph-auto-refresh");
    delete historyQuery.beginTime;
    delete historyQuery.endTime;
    self.plotData = null;
    button.removeClass("btn-default");
    button.addClass("btn-primary");
    if (!onClickButton)
      button.addClass("active");
    $.when(loadHistory()).done(onLoadAllHistory);
  }

  function disableAutoRefresh(onClickButton) {
    var button = $("#item-graph-auto-refresh");
    self.clearAutoReload();
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

  function formatPlotData(item) {
    var i;
    var data = [ { label: item.brief, data:[] } ];

    if (item.unit)
      data[0].label += " [" + item.unit + "]";

    return data;
  };

  function appendPlotData(plotData, history) {
    var i, idx = plotData[0].data.length;
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

  function shiftPlotData(plotData) {
    var beginTimeInMSec, data;

    if (!plotData || !plotData[0])
      return;

    beginTimeInMSec = (self.lastQuery.endTime - self.timeSpan) * 1000;
    data = plotData[0].data;

    while(data.length > 0 && data[0][0] < beginTimeInMSec) {
      if (data[0].length == 1 || data[0][1] > beginTimeInMSec) {
	// remain one point to draw the left edge of the line
	break;
      }
      plotData[0].data.shift();
    }
  }

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

  function getPlotOptions(item, beginTimeInSec, endTimeInSec) {
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
      plotOptions.yaxis.minTickSize = 1;

    return plotOptions;
  }

  function drawGraph(item, plotData) {
    var beginTimeInSec = self.lastQuery.endTime - self.timeSpan;
    var endTimeInSec = self.lastQuery.endTime;
    var plotOptions = getPlotOptions(item, beginTimeInSec, endTimeInSec);

    if (plotData[0].data.length < 3)
      plotOptions.points.show = true;

    self.plotOptions = plotOptions;
    self.plot = $.plot($("#item-graph"), plotData, plotOptions);
  }

  function getTimeRange() {
    var beginTimeInSec, endTimeInSec, date, min;

    if (self.timeRange && historyQuery.endTime)
      return self.timeRange;

    beginTimeInSec = self.lastQuery.endTime - self.timeSpan;
    endTimeInSec = self.lastQuery.endTime;

    // Adjust to 00:00:00
    min = self.lastQuery.endTime - secondsInHour * 24 * 7;
    date = $.plot.dateGenerator(min * 1000, self.plotOptions.xaxis);
    min -= date.getHours() * 3600 + date.getMinutes() * 60 + date.getSeconds();

    self.timeRange = {
      last: [beginTimeInSec, endTimeInSec],
      minSpan: secondsInHour,
      maxSpan: secondsInHour * 24,
      min: min,
      max: self.lastQuery.endTime,
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
        $.when(loadHistory()).done(onLoadAllHistory);
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

  function updateHistory(history) {
    if (!self.plotData)
      self.plotData = formatPlotData(self.item);
    shiftPlotData(self.plotData);
    appendPlotData(self.plotData, history);
  }

  function updateView() {
    self.displayUpdateTime();
    drawGraph(self.item, self.plotData);
    drawSlider();
  }

  function getItemQuery() {
    return 'item?' + $.param(itemQuery);
  };

  function getHistoryQuery() {
    var query = $.extend({}, historyQuery);
    var defaultTimeSpan = secondsInHour * 6;
    var timeSpan = self.timeSpan ? self.timeSpan : defaultTimeSpan;
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
      query.beginTime = query.endTime - timeSpan;

    if (!self.timeSpan)
      self.timeSpan = query.endTime - query.beginTime;
    self.lastQuery = query;

    return 'history?' + $.param(query);
  };

  function buildHostName(item, servers) {
    var server = servers[item.serverId];
    var serverName = getServerName(server, item["serverId"]);
    var hostName   = getHostName(server, item["hostId"]);
    return serverName + ": " + hostName;
  }

  function setItemDescription(item, servers) {
    var hostName = buildHostName(item, servers);
    var title = "";
    title += item.brief;
    title += " (" + hostName + ")";
    $("title").text(title);
    $(".graph h2").text(title);
  }

  function loadItem() {
    var deferred = new $.Deferred;
    self.startConnection(getItemQuery(), function(reply) {
      var items = reply.items;
      var messageDetail;

      if (items && items.length == 1) {
	self.item = items[0];
	self.servers = reply.servers;
        setItemDescription(self.item, self.servers);
        deferred.resolve();
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
        deferred.reject();
      }
    });
    return deferred.promise();
  }

  function loadHistory() {
    var deferred = new $.Deferred;

    self.clearAutoReload();
    self.loadingHistory = true;

    self.startConnection(getHistoryQuery(), function(reply) {
      var maxRecordsPerRequest = 1000;
      var history = reply.history;

      updateHistory(history);
      updateView();

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

  function onLoadAllHistory() {
    if (historyQuery.endTime) {
      disableAutoRefresh();
    } else {
      self.setAutoReload(function() {
        $.when(loadHistory()).done(onLoadAllHistory);
      }, self.reloadIntervalSeconds);
    }
    self.loadingHistory = false;
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
