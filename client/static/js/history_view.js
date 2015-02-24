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

var HistoryView = function(userProfile, options) {
  var self = this;
  var secondsInHour = 60 * 60;

  options = options || {};

  self.reloadIntervalSeconds = 60;
  self.autoReloadIsEnabled = false;
  self.plotData = [];
  self.plotOptions = undefined;
  self.plot = undefined;
  self.timeRange = getTimeRange();
  self.settingSliderTimeRange = false;
  self.loaders = [];
  self.itemSelector = undefined;

  prepare(self.parseQuery(options.query));
  if (self.loaders.length > 0) {
    load();
  } else {
    self.itemSelector.show();
  }

  function prepare(historyQueries) {
    var i;

    self.itemSelector = new HatoholItemSelector({
      view: self,
      appendItemCallback: function(index, query) {
        appendHistoryLoader(query, index);
        load();
      },
      removeItemCallback: function(index) {
        var i;
        var loader = self.itemSelector.getUserData(index);
        for (i = 0; i < self.loaders.length; i++) {
          if (self.loaders[i] == loader) {
            self.loaders.splice(i, 1);
            self.plotData.splice(i, 1);
            updateView();
            return;
          }
        }
      },
    });

    appendGraphArea();
    for (i = 0; i < historyQueries.length; i++)
      appendHistoryLoader(historyQueries[i]);

    updateView();
  }

  function appendGraphArea() {
    $("div.hatohol-graph").append($("<div>", {
      id: "hatohol-graph",
      height: "300px",
    }))
    .append($("<div>", {
      id: "hatohol-graph-slider-area",
    }));

    $("#hatohol-graph-slider-area").append($("<div>", {
      id: "hatohol-graph-slider",
    }))
    .append($("<button>", {
      id: "hatohol-graph-auto-reload",
      type: "button",
      title: gettext("Toggle auto refresh"),
      class: "btn btn-primary glyphicon glyphicon-refresh active",
    }).attr("data-toggle", "button"))
    .append($("<button>", {
      id: "hatohol-graph-settings",
      type: "button",
      title: gettext("Select items"),
      class: "btn btn-default glyphicon glyphicon-cog",
    }).attr("data-toggle", "modal").attr("data-target", "#hatohol-item-list"));

    $("#hatohol-graph").bind("plotselected", function (event, ranges) {
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
      $.plot("#hatohol-graph", self.plotData, plotOptions);

      setSliderTimeRange(Math.floor(ranges.xaxis.from / 1000),
                         Math.floor(ranges.xaxis.to / 1000));

      disableAutoReload();
    });

    // zoom cancel
    $("#hatohol-graph").bind("dblclick", function (event) {
      $.plot("#hatohol-graph", self.plotData, self.plotOptions);
      setSliderTimeRange(Math.floor(self.plotOptions.xaxis.min / 1000),
                         Math.floor(self.plotOptions.xaxis.max / 1000));
    });

    // toggle auto reload
    $("#hatohol-graph-auto-reload").on("click", function() {
      if ($(this).hasClass("active")) {
        disableAutoReload(true);
      } else {
        enableAutoReload(true);
      }
    });
  };

  function createLegendData(item, servers) {
    var legend = { data:[] };

    if (item) {
      // it will be filled by updateTitleAndLegendLabels()
      legend.label = undefind;
      if (item.unit)
        legend.label += " [" + item.unit + "]";
    }

    return legend;
  };

  function updatePlotData() {
    var i, item, servers;
    for (i = 0; i < self.loaders.length; i++) {
      if (self.plotData[i].data) {
        self.plotData[i].data = self.loaders[i].history;
      } else {
        item = self.loaders[i].getItem();
        servers = self.loaders[i].getServers();
        self.plotData[i] = createLegendData(item, servers);
      }
    }
  }

  function appendHistoryLoader(historyQuery, index) {
    var loader = new HistoryLoader({
      view: self,
      defaultTimeSpan: self.timeRange.getSpan(),
      query: historyQuery,
      onLoadItem: function(loader, item, servers) {
        var index = self.itemSelector.getIndexByUserData(loader);
        updatePlotData();
        updateView();
        self.itemSelector.setServers(servers);
        self.itemSelector.setItem(index, item, servers,
                                  loader.options.query.hostgroupId);
      },
      onLoadHistory: function(loader, history) {
        updatePlotData();
        updateView();
      }
    });
    self.loaders.push(loader);
    self.plotData.push(createLegendData());
    if (isNaN(index))
      index = self.itemSelector.appendItem();
    self.itemSelector.setUserData(index, loader);
    if (self.loaders.length == 1)
      initTimeRange();
  }

  function initTimeRange() {
    var emdTime, timeSpan;

    // TODO: allow different time ranges?
    if (!self.loaders.length)
      return;
    endTime = self.loaders[0].options.query.endTime;
    timeSpan = self.loaders[0].getTimeSpan();

    if (endTime)
      self.timeRange.set(endTime - timeSpan, endTime);
    self.autoReloadIsEnabled = !endTime;
  }

  function load() {
    var promises;
    var i;

    self.clearAutoReload();
    if (self.autoReloadIsEnabled) {
      self.timeRange.setEndTime(Math.floor(new Date().getTime() / 1000));
      for (i = 0; i < self.loaders.length; i++)
        self.loaders[i].setTimeRange(undefined, self.timeRange.end, true);
    }

    promises = $.map(self.loaders, function(loader) { return loader.load(); });
    $.when.apply($, promises).done(function() {
      if (self.autoReloadIsEnabled)
        self.setAutoReload(load, self.reloadIntervalSeconds);
    });
  }

  function enableAutoReload(onClickButton) {
    var button = $("#hatohol-graph-auto-reload");

    button.removeClass("btn-default");
    button.addClass("btn-primary");
    if (!onClickButton)
      button.addClass("active");

    self.autoReloadIsEnabled = true;
    load();
  }

  function disableAutoReload(onClickButton) {
    var button = $("#hatohol-graph-auto-reload");
    self.clearAutoReload();
    self.autoReloadIsEnabled = false;
    if (!onClickButton)
      button.removeClass("active");
    button.removeClass("btn-primary");
    button.addClass("btn-default");
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

  function drawGraph() {
    var i;

    self.plotOptions = getPlotOptions(self.timeRange.begin, self.timeRange.end);
    fixupYAxisMapping(self.plotData, self.plotOptions);

    for (i = 0; i < self.plotData.length; i++) {
      if (self.plotData[i].data.length == 1)
        self.plotOptions.points.show = true;
    }

    self.plot = $.plot($("#hatohol-graph"), self.plotData, self.plotOptions);
  }

  function getTimeRange() {
    var timeRange = {
      begin: undefined,
      end: undefined,
      minSpan: secondsInHour,
      maxSpan: secondsInHour * 24,
      min: undefined,
      max: undefined,
      set: function(beginTime, endTime) {
        this.setEndTime(endTime);
        this.begin = beginTime;
        if (this.getSpan() > this.maxSpan)
          this.begin = this.end - this.maxSpan;
        if (this.getSpan() < this.minSpan) {
          if (this.begin + this.minSpan >= this.max) {
            this.end = this.max;
            this.begin = this.max - this.minSpan;
          } else {
            this.end = this.begin + this.minSpan;
          }
        }
      },
      setEndTime: function(endTime) {
        var span = this.getSpan();
        this.end = endTime;
        this.begin = this.end - span;
        if (!this.max || this.end > this.max) {
          this.max = this.end;
          this._adjustMin();
        }
      },
      _adjustMin: function() {
        var date;
        // 1 week ago
        this.min = this.max - secondsInHour * 24 * 7;
        // Adjust to 00:00:00
        date = getDate(this.min * 1000);
        this.min -=
          date.getHours() * 3600 +
          date.getMinutes() * 60 +
          date.getSeconds();
      },
      getSpan: function() {
        if (this.begin && this.end)
          return this.end - this.begin;
        else
          return secondsInHour * 6;
      }
    };
    return timeRange;
  }

  function drawSlider() {
    var timeRange = self.timeRange;

    $("#hatohol-graph-slider").slider({
      range: true,
      min: timeRange.min,
      max: timeRange.max,
      values: [timeRange.begin, timeRange.end],
      change: function(event, ui) {
        var i;

        if (self.settingSliderTimeRange)
          return;
        if (self.isLoading())
          return;

        timeRange.set(ui.values[0], ui.values[1]);
        setSliderTimeRange(timeRange.begin, timeRange.end);
        setGraphTimeRange(timeRange.begin, timeRange.end);
        for (i = 0; i < self.loaders.length; i++)
          self.loaders[i].setTimeRange(timeRange.begin, timeRange.end);
        disableAutoReload();
        load();
        $("#hatohol-graph-auto-reload").removeClass("active");
      },
      slide: function(event, ui) {
        var beginTime = ui.values[0], endTime = ui.values[1];

        if (timeRange.begin != ui.values[0])
          endTime = ui.values[0] + timeRange.getSpan();
        if (ui.values[1] - ui.values[0] < timeRange.minSpan)
          beginTime = ui.values[1] - timeRange.minSpan;
        timeRange.set(beginTime, endTime);
        setSliderTimeRange(timeRange.begin, timeRange.end);
      },
    });
    $("#hatohol-graph-slider").slider('pips', {
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
    $("#hatohol-graph-slider").slider('float', {
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
    values = $("#hatohol-graph-slider").slider("values");
    if (min != values[0])
      $("#hatohol-graph-slider").slider("values", 0, min);
    if (max != values[1])
      $("#hatohol-graph-slider").slider("values", 1, max);
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

  function updateView() {
    updateTitleAndLegendLabels();
    drawGraph();
    drawSlider();
    self.displayUpdateTime();
  }

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

  function updateTitleAndLegendLabels() {
    var i, title, item, servers, loader = self.loaders[0];

    if (self.plotData.length == 0) {
      title = undefined;
    } else if (self.plotData.length == 1) {
      title = buildTitle(loader.getItem(), loader.getServers());
    } else if (isSameHost()) {
      title = buildHostName(loader.getItem(), loader.getServers());
      for (i = 0; i < self.plotData.length; i++) {
        // omit host names in legend labels
        item = self.loaders[i].getItem();
        self.plotData[i].label = getItemBriefWithUnit(item);
      }
    } else if (isSameItem()) {
      title = getItemBriefWithUnit(loader.getItem());
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

    if (title) {
      $("title").text(title);
      $("h2.hatohol-graph").text(title);
    } else {
      $("title").text(gettext("History"));
      $("h2.hatohol-graph").text("");
    }
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
