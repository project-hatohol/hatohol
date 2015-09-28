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
  //
  // options has the following parameters.
  //
  // columnDefinitions: <array> [mandatory]
  //   Choices for visible columns.
  //   e.g.)
  //   {
  //     "monitoringServerName": {
  //       header: gettext("Monitoring Server")
  //     },
  //     ...
  //   }
  //
  // loadedCallback: <function> [optional]
  //   A callback function that is called on the loaded.
  //
  // savedCallback: <function> [optional]
  //   A callback function that is called on the saved.
  //
 var self = this;
  var minAutoReloadInterval = 5;
  var maxAutoReloadInterval = 600;
  var key;

  HatoholUserConfig.apply(this, [options]);
  self.options = options;
  self.config = self.getDefaultConfig();
  self.servers = null;

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

  $('#column-selector-up').click(function() {
    var selected = $("#column-selector-selected option:selected");
    if (!selected || selected.length != 1)
      return;
    selected.insertBefore(selected.prev());
  });

  $('#column-selector-down').click(function() {
    var selected = $("#column-selector-selected option:selected");
    if (!selected || selected.length != 1)
      return;
    selected.insertAfter(selected.next());
  });

  $("#config-save").click(function() {
    self.saveAll();
  });

  var filterSelectors = [
    "incident", "status", "severity", "server", "hostgroup", "host"
  ];
  $.map(filterSelectors, function(selector) {
    $('#' + selector + '-filter-selector').multiselect({
      right: "#" + selector + "-filter-selector-selected",
    });
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

HatoholEventsViewConfig.prototype.saveValue = function(key, value) {
  var self = this;

  self.config[key] = "" + value;

  self.store({
    items: self.config,
    successCallback: function(reply) {
    },
    connectErrorCallback: function(XMLHttpRequest, textStatus, errorThrown) {
      self.showXHRError(XMLHttpRequest);
    },
  });
};

HatoholEventsViewConfig.prototype.reset = function() {
  var self = this;
  var autoReloadInterval = self.getValue('events.auto-reload.interval');
  var key;

  resetAutoReloadInterval();
  resetNumRowsPerPage();
  resetColumnSelector();
  self.setCurrentFilter();

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
    'events.auto-reload.interval': "60",
    'events.num-rows-per-page': "300",
    'events.columns':
      "incidentStatus,status,severity,time," +
      "monitoringServerName,hostName,description",
    'events.sort.type': "time",
    'events.sort.order': "" + hatohol.DATA_QUERY_OPTION_SORT_DESCENDING,
  };
};

HatoholEventsViewConfig.prototype.setServers = function(servers) {
  this.servers = servers;
}

HatoholEventsViewConfig.prototype.setCurrentFilter = function(filter) {
  var self = this;

  var incidents = [
    { value: "NONE",        label: gettext("NONE") },
    { value: "HOLD",        label: gettext("HOLD") },
    { value: "IN PROGRESS", label: gettext("IN PROGRESS") },
    { value: "DONE",        label: gettext("DONE") },
  ];
  var statuses = [
    { value: "0", label: gettext("OK") },
    { value: "1", label: gettext("Problem") },
    { value: "2", label: gettext("Unknown") },
    { value: "3", label: gettext("Notification") },
  ];
  var severities = [
    { value: "0", label: gettext("Not classified") },
    { value: "1", label: gettext("Information") },
    { value: "2", label: gettext("Warning") },
    { value: "3", label: gettext("Average") },
    { value: "4", label: gettext("High") },
    { value: "5", label: gettext("Disaster") },
  ];
  var serverKey, groupKey, hostKey, servers = [], hostgroups = [], hosts = [];

  for (serverKey in self.servers) {
    servers.push({
      value: serverKey,
      label: getNickName(self.servers[serverKey], serverKey),
    });

    for (groupKey in self.servers[serverKey].groups) {
      hostgroups.push({
        value: buildComplexId(serverKey, groupKey),
        label: getHostgroupName(self.servers[serverKey], groupKey) + 
          " (" + getNickName(self.servers[serverKey], serverKey) + ")",
      });
    }

    for (hostKey in self.servers[serverKey].hosts) {
      hosts.push({
        value: buildComplexId(serverKey, hostKey),
        label:
          getHostName(self.servers[serverKey], hostKey) +
          " (" + getNickName(self.servers[serverKey], serverKey) + ")",
      });
    }
  }

  resetSelector("incident", incidents);
  resetSelector("status", statuses);
  resetSelector("severity", severities);
  resetSelector("server", servers);
  resetSelector("hostgroup", hostgroups);
  resetSelector("host", hosts);

  function buildComplexId(serverId, id) {
    // We don't need escape the separator (","). Because the serverId is a
    // number, the first "," is always a separator, not a part of the later ID.
    return "" + serverId + "," + id;
  }

  function resetSelector(filterName, choices) {
    var i;

    $("#" + filterName + "-filter-selector").empty();
    $("#" + filterName + "-filter-selector-selected").empty();

    // set candidate columns (left side)
    for (i = 0; i < choices.length; i++) {
      addItem(choices[i], "#" + filterName + "-filter-selector");
    }

    function addItem(choice, parentId) {
      $("<option/>", {
        text: choice.label,
        value: choice.value,
      }).appendTo(parentId);
    }
  }
}

HatoholEventsViewConfig.prototype.showXHRError = function (XMLHttpRequest) {
  var errorMsg = "Error: " + XMLHttpRequest.status + ": " +
    XMLHttpRequest.statusText;
  hatoholErrorMsgBox(errorMsg);
};
