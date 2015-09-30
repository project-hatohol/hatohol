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
  self.multiselectFilterTypes = [
    "incident", "status", "severity", "server", "hostgroup", "host"
  ];
  self.filterList = [self.getDefaultFilterSettings()];
  self.selectedFilterSettings = null;
  self.removedFilters = {};

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
      title: options.columnDefinitions[key].header,
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

  $.map(self.multiselectFilterTypes, function(selector) {
    $('#' + selector + '-filter-selector').multiselect({
      right: "#" + selector + "-filter-selector-selected",
    });
  });

  function quickHostFilter(word) {
    var setVisible = function(option) {
      var obj = $(option);
      var text = obj.text().toLowerCase();
      if (word.length == 0 || text.indexOf(word.toLowerCase()) >= 0) {
	obj.show();
      } else {
	obj.hide();
      }
    };
    $.map($("#host-filter-selector option"), setVisible)
    $.map($("#host-filter-selector-selected option"), setVisible)
  }

  $("#quick-host-search-submit").click(function() {
    var word = $("#quick-host-search-entry").val();
    quickHostFilter(word);
  });

  $("#quick-host-search-clear").click(function() {
    $("#quick-host-search-entry").val("");
    quickHostFilter("");
  });

  $("#filter-name-entry").change(function () {
    $.extend(self.selectedFilterSettings,
             self.getCurrentFilterSettings());
    self.resetFilterList();
  });

  $("#filter-name-list-delete-button").click(function () {
    var i, removedFilters;
    for (i = 0; i < self.filterList.length; i++) {
      if (self.filterList[i] === self.selectedFilterSettings) {
        removedFilters = self.filterList.splice(i, 1);
        if (removedFilters.length == 1 && removedFilters[0].id)
          self.removedFilters[removedFilters[0].id] = true;
	if (self.filterList.length == 0)
	  self.filterList.push(self.getDefaultFilterSettings());
        self.setCurrentFilterSettings(self.filterList[0]);
        self.resetFilterList();
        return;
      }
    }
  });

  self.loadAll();
};

HatoholEventsViewConfig.prototype = Object.create(HatoholUserConfig.prototype);
HatoholEventsViewConfig.prototype.constructor = HatoholEventsViewConfig;

HatoholEventsViewConfig.prototype.loadAll = function() {
  var self = this;
  var configDeferred = new $.Deferred;
  var filterDeferred = new $.Deferred;

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
    },
    connectErrorCallback: function(XMLHttpRequest, textStatus, errorThrown) {
      self.showXHRError(XMLHttpRequest);
    },
    completionCallback: function() {
      configDeferred.resolve();
    },
  });

  new HatoholConnector({
    pathPrefix: '',
    url: '/event-filters',
    replyCallback: function(reply, parser) {
      if (reply.length > 0)
        self.filterList = reply;
      else
        self.filterList = [self.getDefaultFilterSettings()];
    },
    parseErrorCallback: function(reply, parser) {
      hatoholErrorMsgBoxForParser(reply, parser);
    },
    completionCallback: function() {
      filterDeferred.resolve();
    },
  });

  $.when(configDeferred.promise(), filterDeferred.promise()).done(function() {
    self.removedFilters = {};
    if (self.options.loadedCallback)
      self.options.loadedCallback(self);
  });
};

HatoholEventsViewConfig.prototype.saveAll = function() {
  var self = this;
  var deferreds = [new $.Deferred];

  $.extend(self.config, {
    'events.auto-reload.interval': $("#auto-reload-interval").val(),
    'events.num-rows-per-page': $("#num-rows-per-page").val(),
    'events.columns': buildSelectedColumns(),
  });

  $.extend(self.selectedFilterSettings,
           self.getCurrentFilterSettings());

  self.store({
    items: self.config,
    successCallback: function(reply) {
      $("#events-view-config").modal("hide");
    },
    connectErrorCallback: function(XMLHttpRequest, textStatus, errorThrown) {
      self.showXHRError(XMLHttpRequest);
    },
    completionCallback: function() {
      deferreds[0].resolve();
    },
  });

  $.map(self.filterList, function(filter) {
    var path = "/event-filters/";
    var deferred = new $.Deferred;

    if (filter.id)
      path += filter.id;

    deferreds.push(deferred);
    new HatoholConnector({
      pathPrefix: "",
      url: path,
      request: filter.id ? "PUT" : "POST",
      data: JSON.stringify(filter),
      replyCallback: function(reply, parser) {
        $.extend(filter, reply);
      },
      parseErrorCallback: function(reply, parser) {
        hatoholErrorMsgBoxForParser(reply, parser);
      },
      completionCallback: function() {
        deferred.resolve();
      },
    });
  });

  $.map(self.removedFilters, function(val, id) {
    var deferred = new $.Deferred;

    new HatoholConnector({
      pathPrefix: "",
      url: "/event-filters/" + id,
      request: "DELETE",
      replyCallback: function(reply, parser) {
        delete self.removedFilters[id]
      },
      parseErrorCallback: function(reply, parser) {
        hatoholErrorMsgBoxForParser(reply, parser);
      },
      completionCallback: function() {
        deferred.resolve();
      },
    });
  });

  var promises = $.map(deferreds, function(deferred) {
    return deferred.promise();
  });
  $.when.apply($, promises).done(function() {
    if (self.options.savedCallback)
      self.options.savedCallback(self);
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
  self.resetFilterList();
  self.setCurrentFilterSettings(self.filterList[0]);

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
        title: definitions[key].header,
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
};

HatoholEventsViewConfig.prototype.resetFilterList = function(filter) {
  var self = this;
  var filters = self.filterList;
  $("#filter-name-list").empty();

  $.map(filters, function(filter) {
    $("<li/>").append(
      $("<a>", {
        text: filter.name,
        href: "#",
        click: function(obj) {
          self.selectedFilterNameElement = this;
          self.setCurrentFilterSettings(filter);
          self.resetFilterList();
        },
      })
    ).appendTo("#filter-name-list");
  });

  $("<li/>", {"class": "divider"}).appendTo("#filter-name-list");

  $("<li/>").append(
    $("<a>", {
      text: gettext("Add a new filter"),
      href: "#",
      click: function(obj) {
        var newFilter = self.getDefaultFilterSettings();
        newFilter.name = gettext("New filter");
        self.filterList.push(newFilter);
        self.setCurrentFilterSettings(newFilter);
        self.resetFilterList();
      },
    })
  ).appendTo("#filter-name-list");
};

HatoholEventsViewConfig.prototype.setCurrentFilterSettings = function(filter) {
  var self = this;

  var candidates = {
    incident: [
      { value: "NONE",        label: gettext("NONE") },
      { value: "HOLD",        label: gettext("HOLD") },
      { value: "IN PROGRESS", label: gettext("IN PROGRESS") },
      { value: "DONE",        label: gettext("DONE") },
    ],
    status: [
      { value: "0", label: gettext("OK") },
      { value: "1", label: gettext("Problem") },
      { value: "2", label: gettext("Unknown") },
      { value: "3", label: gettext("Notification") },
    ],
    severity: [
      { value: "0", label: gettext("Not classified") },
      { value: "1", label: gettext("Information") },
      { value: "2", label: gettext("Warning") },
      { value: "3", label: gettext("Average") },
      { value: "4", label: gettext("High") },
      { value: "5", label: gettext("Disaster") },
    ],
    server: [],
    hostgroup: [],
    host: [],
  };
  var serverKey, groupKey, hostKey;

  filter = filter || {};

  if (self.selectedFilterSettings)
    $.extend(self.selectedFilterSettings,
             self.getCurrentFilterSettings());

  $("#filter-name-entry").val(filter.name);
  $("#period-filter-setting").val(filter.days);

  // collect candidate servers/hostgroups/hosts
  for (serverKey in self.servers) {
    candidates.server.push({
      value: serverKey,
      label: getNickName(self.servers[serverKey], serverKey),
    });

    for (groupKey in self.servers[serverKey].groups) {
      candidates.hostgroup.push({
        value: buildComplexId(serverKey, groupKey),
        label:
          getNickName(self.servers[serverKey], serverKey) + ": " +
          getHostgroupName(self.servers[serverKey], groupKey)
      });
    }

    for (hostKey in self.servers[serverKey].hosts) {
      candidates.host.push({
        value: buildComplexId(serverKey, hostKey),
        label:
          getNickName(self.servers[serverKey], serverKey) + ": " +
          getHostName(self.servers[serverKey], hostKey)
      });
    }
  }

  $.map(self.multiselectFilterTypes, function(type) {
    resetSelector(type, candidates[type], filter[type]);
  });

  function buildComplexId(serverId, id) {
    // We don't need escape the separator (","). Because the serverId is a
    // number, the first "," is always a separator, not a part of the later ID.
    return "" + serverId + "," + id;
  }

  function collectSelectedValues(list) {
    var selected = {};

    if (!(list instanceof Array))
      return selected;

    $.map(list, function(item) {
      if (typeof item == "string") {
        selected[item] = true;
      } else if (typeof item == "object") {
        selected[buildComplexId(item.serverId, item.id)] = true;
      }
    });
    return selected;
  }

  function resetSelector(filterName, choices, config) {
    var i, selected, exclude;

    config = config || {};
    selected = collectSelectedValues(config.selected);
    exclude = config.exclude ? "1" : "0";

    $("#" + filterName + "-filter-selector").empty();
    $("#" + filterName + "-filter-selector-selected").empty();

    $("#enable-" + filterName + "-filter-selector").prop("checked", config.enable);
    $("input:radio" +
      "[name=" + filterName + "-filter-select-options]" +
      "[value=" + exclude + "]").attr('checked', true);

    // set candidate items (left side)
    for (i = 0; i < choices.length; i++) {
      if (!selected[choices[i].value])
        addItem(choices[i], "#" + filterName + "-filter-selector");
    }

    // set selected items (right side)
    for (i = 0; i < choices.length; i++) {
      if (selected[choices[i].value])
        addItem(choices[i], "#" + filterName + "-filter-selector-selected");
    }

    function addItem(choice, parentId) {
      $("<option/>", {
        text: choice.label,
        title: choice.label,
        value: choice.value,
      }).appendTo(parentId);
    }
  }

  self.selectedFilterSettings = filter;
};

HatoholEventsViewConfig.prototype.getDefaultFilterSettings = function() {
  return {
    name: gettext("ALL (31 days)"),
    days: 31,
    incident: {
      enable: false,
      selected: []
    },
    status: {
      enable: false,
      selected: []
    },
    severity: {
      enable: false,
      selected: []
    },
    server: {
      enable: false,
      exclude: false,
      selected: []
      },
    hostgroup: {
      enable: false,
      exclude: false,
      selected: []
    },
    host: {
      enable: false,
      exclude: false,
      selected: []
    }
  };
};


HatoholEventsViewConfig.prototype.getCurrentFilterSettings = function() {
  var self = this;
  var filter = {};

  filter.name = $("#filter-name-entry").val();
  filter.days = parseInt($("#period-filter-setting").val());
  if (isNaN(filter.days))
    filter.days = 0;
  $.map(self.multiselectFilterTypes, function(type) {
    addFilterSetting(type);
  });

  function addFilterSetting(filterName) {
    var config = {};
    config.enable = $("#enable-" + filterName +
                      "-filter-selector").prop("checked");

    var exclude = $("input:radio[name=" + filterName +
                    "-filter-select-options]:checked").val();
    if (exclude == "1")
      config.exclude = true;
    else if (exclude == "0")
      config.exclude = false;

    var selected = $("#" + filterName + "-filter-selector-selected option");
    config.selected = $.map(selected, function(option) {
      return $(option).val();
    });

    filter[filterName] = config;
  }

  return filter;
};

HatoholEventsViewConfig.prototype.showXHRError = function (XMLHttpRequest) {
  var errorMsg = "Error: " + XMLHttpRequest.status + ": " +
    XMLHttpRequest.statusText;
  hatoholErrorMsgBox(errorMsg);
};
