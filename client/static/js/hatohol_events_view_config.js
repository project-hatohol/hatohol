/*
 * Copyright (C) 2015 Project Hatohol
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

var HatoholEventsViewConfig = function(options) {
  var self = this;
  var minAutoReloadInterval = 5;
  var maxAutoReloadInterval = 600;
  var key;

  HatoholUserConfig.apply(this, options);
  self.options = options;
  self.config = self.getDefaultConfig();

  $('#events-view-config').on('show.bs.modal', function (event) {
    self.reset();
  });

  $("#auto-reload-interval-slider").slider({
    min: minAutoReloadInterval,
    max: maxAutoReloadInterval,
    step: 1,
    slide: function(event, ui) {
      $("#auto-reload-interval").val(ui.value);
    },
  });

  $("#auto-reload-interval").change(function() {
    var value = $("#auto-reload-interval").val();

    if (value < minAutoReloadInterval) {
      value = minAutoReloadInterval;
      $("#auto-reload-interval").val(value);
    }
    if (value > maxAutoReloadInterval) {
      value = maxAutoReloadInterval;
      $("#auto-reload-interval").val(value);
    }

    $("#auto-reload-interval-slider").slider("value", value);
  });

  for (key in options.columnDefinitions) {
    $("<option/>", {
      text: options.columnDefinitions[key].header,
      value: key,
    }).appendTo("#column-selector");
  }
  $('#column-selector').multiselect({
    sort: false,
    right: "#column-selector-selected",
  });

  $("#config-save").click(function() {
    self.saveAll();
  });

  self.loadAll();
};

HatoholEventsViewConfig.prototype = Object.create(HatoholUserConfig.prototype);
HatoholEventsViewConfig.prototype.constructor = HatoholEventsViewConfig;

HatoholEventsViewConfig.prototype.loadAll = function() {
  var self = this;
  self.get({
    itemNames: [
      'events.auto-reload.interval',
      'events.num-rows-per-page',
      'events.sort.type',
      'events.sort.order',
      'events.columns',
    ],
    successCallback: function(config) {
      $.extend(self.config, config);
      self.reset();
      if (self.options.loadedCallback)
        self.options.loadedCallback(self);
    },
    connectErrorCallback: function(XMLHttpRequest, textStatus, errorThrown) {
      self.showXHRError(XMLHttpRequest);
    },
  });
};

HatoholEventsViewConfig.prototype.saveAll = function() {
  var self = this;

  $.extend(self.config, {
    'events.auto-reload.interval': $("#auto-reload-interval").val(),
    'events.num-rows-per-page': $("#num-rows-per-page").val(),
    'events.columns': buildSelectedColumns(),
  });

  self.store({
    items: self.config,
    successCallback: function(reply) {
      $("#events-view-config").modal("hide");

      if (self.options.savedCallback)
        self.options.savedCallback(self);
    },
    connectErrorCallback: function(XMLHttpRequest, textStatus, errorThrown) {
      self.showXHRError(XMLHttpRequest);
    },
  });

  function buildSelectedColumns() {
    var selected = $("#column-selector-selected option");
    var values = $.map(selected, function(option) { return $(option).val(); });
    return values.join(',');
  }
};

HatoholEventsViewConfig.prototype.getValue = function(key) {
    var defaultConfig = this.getDefaultConfig();
    return this.findOrDefault(this.config, key, defaultConfig[key]);
};

HatoholEventsViewConfig.prototype.reset = function() {
  var self = this;
  var autoReloadInterval = self.getValue('events.auto-reload.interval');
  var key;

  resetAutoReloadInterval();
  resetNumRowsPerPage();
  resetColumnSelector();

  function resetAutoReloadInterval() {
    $("#auto-reload-interval-slider").slider("value", autoReloadInterval);
    $("#auto-reload-interval").val(autoReloadInterval);
  }

  function resetNumRowsPerPage() {
    $("#num-rows-per-page").val(self.getValue('events.num-rows-per-page'));
  }

  function resetColumnSelector() {
    var definitions = self.options.columnDefinitions;
    var columns = self.getValue('events.columns').split(",");
    var selectedColumns = {}, i, column, def;

    $("#column-selector").empty();
    $("#column-selector-selected").empty();

    // set selected columns (right side)
    for (i = 0; i < columns.length; i++) {
      column = columns[i];
      def = definitions[columns[i]];
      selectedColumns[column] = true;
      if (!def)
        continue;
      if (column in definitions)
        addItem(column, "#column-selector-selected");
    }

    // set candidate columns (left side)
    for (key in definitions) {
      if (key in selectedColumns)
        continue;
      addItem(key, "#column-selector");
    }

    function addItem(key, parentId) {
      $("<option/>", {
        text: definitions[key].header,
        value: key,
      }).appendTo(parentId);
    }
  }
};

HatoholEventsViewConfig.prototype.getDefaultConfig = function() {
  return {
    'events.auto-reload.interval': 60,
    'events.num-rows-per-page': 300,
    'events.columns':
      "incidentStatus,status,severity,time," +
      "monitoringServerName,hostName,description",
    'events.sort.type': "time",
    'events.sort.order': hatohol.DATA_QUERY_OPTION_SORT_DESCENDING,
  };
};

HatoholEventsViewConfig.prototype.showXHRError = function (XMLHttpRequest) {
  var errorMsg = "Error: " + XMLHttpRequest.status + ": " +
    XMLHttpRequest.statusText;
  hatoholErrorMsgBox(errorMsg);
};
