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

var HatoholTimeRangeSelector = function(options) {
  var self = this;
  var secondsInHour = 60 * 60;

  options = options || {};

  self.id = options.id;
  self.options = options;
  self.timeRange = getTimeRange();
  self.settingTimeRange = false;

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

  function getDate(timeMSec) {
    var plotOptions = {
      mode: "time",
      timezone: "browser"
    };
    return $.plot.dateGenerator(timeMSec, plotOptions);
  }
}

HatoholTimeRangeSelector.prototype.draw = function() {
  var self = this;
  var secondsInHour = 60 * 60;
  var timeRange = self.timeRange;

  $("#" + self.id).slider({
    range: true,
    min: timeRange.min,
    max: timeRange.max,
    values: [timeRange.begin, timeRange.end],
    change: function(event, ui) {
      var i;

      if (self.settingTimeRange)
        return;

      self.setTimeRange(ui.values[0], ui.values[1]);

      if (self.options.setTimeRangeCallback)
        self.options.setTimeRangeCallback(timeRange.begin, timeRange.end);
    },
    slide: function(event, ui) {
      var beginTime = ui.values[0], endTime = ui.values[1];

      if (timeRange.begin != ui.values[0])
        endTime = ui.values[0] + timeRange.getSpan();
      if (ui.values[1] - ui.values[0] < timeRange.minSpan)
        beginTime = ui.values[1] - timeRange.minSpan;
      self.setTimeRange(beginTime, endTime);
    },
  });
  $("#" + self.id).slider('pips', {
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
  $("#" + self.id).slider('float', {
    formatLabel: function(val) {
      return formatDate(val);
    },
  });
}

HatoholTimeRangeSelector.prototype.setTimeRange = function(minSec, maxSec) {
  var self = this;
  var values;

  if (self.settingTimeRange)
    return;

  self.settingTimeRange = true;
  self.timeRange.set(minSec, maxSec);
  values = $("#" + self.id).slider("values");
  if (self.timeRange.begin != values[0])
    $("#" + self.id).slider("values", 0, self.timeRange.begin);
  if (self.timeRange.end != values[1])
    $("#" + self.id).slider("values", 1, self.timeRange.end);
  self.settingTimeRange = false;
}

HatoholTimeRangeSelector.prototype.getTimeSpan = function() {
  return this.timeRange.getSpan();
}

HatoholTimeRangeSelector.prototype.getBeginTime = function() {
  return this.timeRange.begin;
}

HatoholTimeRangeSelector.prototype.getEndTime = function() {
  return this.timeRange.end;
}


var HistoryView = function(userProfile, options) {
  var self = this;
  var secondsInHour = 60 * 60;

  options = options || {};

  self.reloadIntervalSeconds = 60;
  self.autoReloadIsEnabled = false;
  self.graph = undefined;
  self.slider = undefined;
  self.itemSelector = undefined;
  self.loaders = [];

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
        var loader = self.itemSelector.getUserData(index);
        removeHistoryLoader(loader);
        updateView();
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

    // toggle auto reload
    $("#hatohol-graph-auto-reload").on("click", function() {
      if ($(this).hasClass("active")) {
        disableAutoReload(true);
      } else {
        enableAutoReload(true);
      }
    });

    self.graph = new HatoholGraph({
      id: "hatohol-graph",
      zoomCallback: function(minSec, maxSec) {
        self.slider.setTimeRange(minSec, maxSec);
        disableAutoReload();
      },
    });
    self.slider = new HatoholTimeRangeSelector({
      id: "hatohol-graph-slider",
      setTimeRangeCallback: function(minSec, maxSec) {
        var i;
        if (self.isLoading())
          return;
        self.graph.draw(minSec, maxSec);
        for (i = 0; i < self.loaders.length; i++)
          self.loaders[i].setTimeRange(minSec, maxSec);
        disableAutoReload();
        load();
        $("#hatohol-graph-auto-reload").removeClass("active");
      },
    });
  };

  function appendHistoryLoader(historyQuery, index) {
    var loader = new HatoholHistoryLoader({
      view: self,
      defaultTimeSpan: self.slider.getTimeSpan(),
      query: historyQuery,
      onLoadItem: function(loader, item, servers) {
        var index = self.itemSelector.getIndexByUserData(loader);
        updateView();
        self.itemSelector.setServers(servers);
        self.itemSelector.setItem(index, item, servers,
                                  loader.options.query.hostgroupId);
      },
      onLoadHistory: function(loader, history) {
        updateView();
      }
    });
    self.loaders.push(loader);
    self.graph.appendHistoryLoader(loader);
    if (isNaN(index))
      index = self.itemSelector.appendItem();
    self.itemSelector.setUserData(index, loader);
    if (self.loaders.length == 1)
      initTimeRange();
  }

  function removeHistoryLoader(loader) {
    var i;
    for (i = 0; i < self.loaders.length; i++) {
      if (loader) {
        self.loaders.splice(i, 1);
        break;
      }
    }
    self.graph.removeHistoryLoader(loader);
  }

  function initTimeRange() {
    var emdTime, timeSpan;

    // TODO: allow different time ranges?
    if (!self.loaders.length)
      return;
    endTime = self.loaders[0].options.query.endTime;
    timeSpan = self.loaders[0].getTimeSpan();

    if (endTime)
      self.setTimeRange(endTime - timeSpan, endTime);
    self.autoReloadIsEnabled = !endTime;
  }

  function load() {
    var promises;
    var i;
    var endTime = Math.floor(new Date().getTime() / 1000);
    var beginTime = endTime - self.slider.getTimeSpan();

    self.clearAutoReload();
    if (self.autoReloadIsEnabled) {
      self.slider.setTimeRange(beginTime, endTime);
      for (i = 0; i < self.loaders.length; i++)
        self.loaders[i].setTimeRange(undefined, self.slider.getEndTime(), true);
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

  function setTitle(title) {
    if (title) {
      $("title").text(title);
      $("h2.hatohol-graph").text(title);
    } else {
      $("title").text(gettext("History"));
      $("h2.hatohol-graph").text("");
    }
  }

  function updateView() {
    self.graph.draw(self.slider.getBeginTime(),
                    self.slider.getEndTime());
    self.slider.draw();
    setTitle(self.graph.title);
    self.displayUpdateTime();
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
