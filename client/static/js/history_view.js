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

  self.options = options || {};
  self.reloadIntervalSeconds = 60;
  self.autoReloadIsEnabled = false;
  self.graph = undefined;
  self.slider = undefined;
  self.itemSelector = undefined;
  self.loaders = [];

  prepare(self.parseQuery(self.options.query));
  if (self.loaders.length > 0) {
    load();
  } else {
    self.itemSelector.show();
  }

  function prepare(historyQueries) {
    var i;

    appendWidgets();
    for (i = 0; i < historyQueries.length; i++)
      appendHistoryLoader(historyQueries[i]);
    updateView();
  }

  function appendWidgets() {
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
      self.slider.setTimeRange(endTime - timeSpan, endTime);
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
