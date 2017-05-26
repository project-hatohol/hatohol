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

var HatoholGraph = function(options) {
  var self = this;

  self.options = options || {};
  self.id = self.options.id || "hatohol-graph";
  self.options = options;
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

      if (self.options.zoomCallback)
        self.options.zoomCallback(Math.floor(ranges.xaxis.from / 1000),
                                  Math.floor(ranges.xaxis.to / 1000));
    });

    // zoom cancel
    $("#" + self.id).bind("dblclick", function (event) {
      var minSec = Math.floor(self.plotOptions.xaxis.min / 1000);
      var maxSec = Math.floor(self.plotOptions.xaxis.max / 1000);

      $.plot("#" + self.id, self.plotData, self.plotOptions);

      if (self.options.zoomCallback)
        self.options.zoomCallback(minSec, maxSec);
    });
  }
};

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
    var i, item, label, axis, axes = [], table = {}, isInteger;
    for (i = 0; i < self.loaders.length; i++) {
      item = self.loaders[i].getItem();
      label = item ? item.unit : "";
      axis = item ? table[item.unit] : undefined;
      isInteger = item && (isInt(item.lastValue));
      if (axis) {
        if (!isInteger)
          delete axis.minTickSize;
      } else {
        axis = getYAxisOptions(label);
        if (isInteger)
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
        max: endTimeInSec * 1000
      },
      yaxes: getYAxesOptions(),
      legend: {
        show: (self.plotData.length > 1),
        position: "sw"
      },
      points: {
      },
      selection: {
        mode: "x"
      }
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
};

HatoholGraph.prototype._createLegendData = function(item, servers) {
  var legend = { data:[] };

  if (item) {
    // it will be filled by updateTitleAndLegendLabels()
    legend.label = undefined;
    if (item.unit)
      legend.label += " [" + item.unit + "]";
  }

  return legend;
};

HatoholGraph.prototype.appendHistoryLoader = function(loader) {
  var self = this;

  self.loaders.push(loader);
  self.plotData.push(self._createLegendData(loader.getItem(),
                                            loader.getServers()));
};

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
};

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
};

HatoholGraph.prototype._updateTitleAndLegendLabels = function() {
  var self = this;

  doUpdate();

  function buildHostName(item, servers) {
    if (!item || !servers)
      return gettext("Unknown host");
    var server = servers[item.serverId];
    var serverName = getNickName(server, item.serverId);
    var hostName   = getHostName(server, item.hostId);
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

    if (self.plotData.length === 0) {
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
};
