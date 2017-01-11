/*
 * Copyright (C) 2015-2016 Project Hatohol
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
  var defaultAutoReloadInterval = 60;
  var minAutoReloadInterval = 5;
  var maxAutoReloadInterval = 600;
  var defaultNumberOfRows = 300;
  var minNumberOfRows = 10;
  var maxNumberOfRows = 500;
  var defaultPeriodDays = 31;
  var minPeriodDays = 1;
  var maxPeriodDays = 3650;
  var key;

  HatoholUserConfig.apply(this, [options]);
  self.options = options;
  self.config = self.getDefaultConfig();
  self.servers = null;
  self.multiselectFilterTypes = [
    "incident", "type", "severity", "server", "hostgroup", "host"
  ];
  self.filterList = [self.getDefaultFilterConfig()];
  self.resetEditingFilterList();
  self.selectedFilterConfig = null;
  self.removedFilters = {};
  self.defaultFilterName = gettext("New filter");
  self.filterTypeLabelMap = {
    "incident"  : gettext("Handling"),
    "type"      : gettext("Status"),
    "severity"  : gettext("Severity"),
    "server"    : gettext("Monitoring Server"),
    "hostgroup" : gettext("Hostgroup"),
    "host"      : gettext("Host"),
    "column"    : gettext("Display Item")
  };

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
    if (isNaN(value)) {
      value = defaultAutoReloadInterval;
      $("#auto-reload-interval").val(value);
    }

    $("#auto-reload-interval-slider").slider("value", value);
  });

  $("#num-rows-per-page").change(function() {
    var value = $("#num-rows-per-page").val();

    if (value < minNumberOfRows) {
      value = minNumberOfRows;
      $("#num-rows-per-page").val(value);
    }
    if (value > maxNumberOfRows) {
      value = maxNumberOfRows;
      $("#num-rows-per-page").val(value);
    }
    if (isNaN(value)) {
      value = defaultNumberOfRows;
      $("#num-rows-per-page").val(value);
    }
  });

  $("#period-filter-setting").change(function() {
    var value = $("#period-filter-setting").val();

    if (value < minPeriodDays) {
      value = minPeriodDays;
      $("#period-filter-setting").val(value);
    }
    if (value > maxPeriodDays) {
      value = maxPeriodDays;
      $("#period-filter-setting").val(value);
    }
    if (isNaN(value)) {
      value = defaultPeriodDays;
      $("#period-filter-setting").val(value);
    }
  });

  $.map(self.multiselectFilterTypes, function(selector) {
    $("#enable-" + selector + "-filter-selector").change(function() {
      var enabled = $(this).prop("checked");
      if (enabled) {
	$(this).parent().next(".panel").show();
      } else {
	$(this).parent().next(".panel").hide();
      }
    }).change();
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

  $('#all-left-column-selector').click(function() {
    $("#column-selector option").prop("selected", true);
  });

  $('#all-right-column-selector').click(function() {
    $("#column-selector-selected option").prop("selected", true);
  });

  $("#config-save").click(function() {
    var emptyFilterList = getEmptyFilterList();
    if (emptyFilterList.length === 0) {
      self.saveAll();
    } else {
      var body = gettext("The following filter(s) empty.") + "</br>";
      for (var x = 0; x < emptyFilterList.length; x++) {
        body += " - " + self.filterTypeLabelMap[emptyFilterList[x]] + "</br>";
      }
      var errorDialog = new HatoholModal({
        "id": "filter-error-dialog",
        "title": gettext("Fail to saving a filter"),
        "body": body,
        "footer": $(getFooterHTML()),
      });
      $("#close-button").on("click", function() {
        errorDialog.close();
        $('#filter-error-dialog').on('hidden.bs.modal', function () {
          $('body').addClass('modal-open');
        });
      });
      errorDialog.show();
    }
  });

  $.map(self.multiselectFilterTypes, function(selector) {
    $('#' + selector + '-filter-selector').multiselect({
      right: "#" + selector + "-filter-selector-selected",
    });
  });

  function getFooterHTML() {
    var label = gettext("Close");
    var s = '<button type="button" class="btn btn-primary"';
    s += 'id="close-button">' + label + '</button>';
    return s;
  }

  function getEmptyFilterList() {
    var emptyList = [];
    $.map(self.multiselectFilterTypes, function(selector) {
      if ($('#' + selector + '-filter-selector-selected')[0].length < 1 &&
          $('#enable-' + selector + '-filter-selector').prop("checked")) {
        emptyList.push(selector);
      }
    });
    if ($('#column-selector-selected')[0].length < 1){
      emptyList.push("column");
    }
    return emptyList;
  }

  function quickHostFilter(word) {
    var setVisible = function(option) {
      var obj = $(option);
      var text = obj.text().toLowerCase();
      if (word.length === 0 || text.indexOf(word.toLowerCase()) >= 0) {
	obj.show();
      } else {
	obj.hide();
      }
    };
    $.map($("#host-filter-selector option"), setVisible);
    $.map($("#host-filter-selector-selected option"), setVisible);
  }

  function getFilterNameList(){
    var filterNameList = [];
    var anchors = $("#filter-name-list").children("li").find("a");
    for (var i=0; anchors.length - 1 > i; i++) {
      filterNameList.push(anchors[i].text);
    }
    return filterNameList;
  }

  function getUniqueFilterName(filterName) {
    var filterNameList = getFilterNameList(), suffix = 0, uniqueName = filterName;
    function incrementSuffix() {
      suffix += 1;
      return filterName + "("+ String(suffix) +")";
    }

    while (filterNameList.indexOf(uniqueName) != -1)
      uniqueName = incrementSuffix();

    return uniqueName;
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
    var value = $("#filter-name-entry").val();

    if (value.length === 0) {
      value = self.defaultFilterName;
    }
    value = value.slice(0, 128);
    value = getUniqueFilterName(value);
    $("#filter-name-entry").val(value);

    $.extend(self.selectedFilterConfig,
             self.getCurrentFilterConfig());
    self.resetFilterList();
  });

  $("#filter-name-list-delete-button").click(function () {
    var i, removedFilters;
    for (i = 0; i < self.editingFilterList.length; i++) {
      if (self.editingFilterList[i] === self.selectedFilterConfig) {
        removedFilters = self.editingFilterList.splice(i, 1);
        if (removedFilters.length == 1 && removedFilters[0].id)
          self.removedFilters[removedFilters[0].id] = true;
        if (self.editingFilterList.length === 0)
          self.editingFilterList.push(self.getDefaultFilterConfig());
        self.setCurrentFilterConfig(self.editingFilterList[0]);
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
  var configDeferred = new $.Deferred();
  var filterDeferred = new $.Deferred();

  self.get({
    itemNames: Object.keys(self.getDefaultConfig()),
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
      if (reply.length > 0) {
        self.filterList = reply;
      } else {
        self.filterList = [self.getDefaultFilterConfig()];
      }
      self.resetEditingFilterList();
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

  // Synchronize UI & internal data
  $.extend(self.selectedFilterConfig,
           self.getCurrentFilterConfig());

  // Save filters at first to determine filter IDs
  $.when(saveFilters(), removeFilters()).done(function() {
    self.filterList = self.editingFilterList;
    self.resetEditingFilterList();

    // Then save remaining config values
    var idx1 = $("#summary-default-filter-selector").val();
    var idx2 = $("#events-default-filter-selector").val();
    var id1 = idx1 ? self.filterList[idx1].id : 0;
    var id2 = idx2 ? self.filterList[idx2].id : 0;

    $.extend(self.config, {
      'events.auto-reload.interval': $("#auto-reload-interval").val(),
      'events.num-rows-per-page': $("#num-rows-per-page").val(),
      'events.columns': buildSelectedColumns(),
      'events.summary.default-filter-id': "" + id1,
      'events.default-filter-id': "" + id2,
    });

    self.store({
      items: self.config,
      successCallback: function(reply) {
        $("#events-view-config").modal("hide");
      },
      connectErrorCallback: function(XMLHttpRequest, textStatus, errorThrown) {
        self.showXHRError(XMLHttpRequest);
      },
      completionCallback: function() {
        if (self.options.savedCallback)
          self.options.savedCallback(self);
      },
    });
  });

  function buildSelectedColumns() {
    var selected = $("#column-selector-selected option");
    var values = $.map(selected, function(option) { return $(option).val(); });
    return values.join(',');
  }

  function saveFilters() {
    var deferred = new $.Deferred();

    var promises = $.map(self.editingFilterList, function(filter) {
      var path = "/event-filters/";
      var deferred = new $.Deferred();

      if (filter.id)
        path += filter.id;

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

      return deferred.promise();
    });

    $.when.apply($, promises).done(function() {
      deferred.resolve();
    });

    return deferred.promise();
  }

  function removeFilters() {
    var deferred = new $.Deferred();

    var promises = $.map(self.removedFilters, function(val, id) {
      var deferred = new $.Deferred();

      new HatoholConnector({
        pathPrefix: "",
        url: "/event-filters/" + id,
        request: "DELETE",
        replyCallback: function(reply, parser) {
          delete self.removedFilters[id];
        },
        parseErrorCallback: function(reply, parser) {
          hatoholErrorMsgBoxForParser(reply, parser);
        },
        completionCallback: function() {
          deferred.resolve();
        },
      });

      return deferred.promise();
    });

    $.when.apply($, promises).done(function() {
      deferred.resolve();
    });

    return deferred.promise();
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

  self.resetEditingFilterList();
  self.selectedFilterConfig = null;
  self.removedFilters = {};

  resetAutoReloadInterval();
  resetNumRowsPerPage();
  resetColumnSelector();
  self.resetFilterList();
  self.setCurrentFilterConfig(self.editingFilterList[0]);

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
      "incidentStatus,type,severity,time," +
      "monitoringServerName,hostName,description,userComment",
    'events.sort.type': "time",
    'events.sort.order': "" + hatohol.DATA_QUERY_OPTION_SORT_DESCENDING,
    'events.default-filter-id': "0",
    'events.summary.default-filter-id': "0",
  };
};

HatoholEventsViewConfig.prototype.setServers = function(servers) {
  this.servers = servers;
};

HatoholEventsViewConfig.prototype.resetEditingFilterList = function() {
  var self = this;
  self.editingFilterList = [];
  $.map(self.filterList, function(filter) {
    self.editingFilterList.push($.extend({}, filter));
  });
};

HatoholEventsViewConfig.prototype.resetFilterList = function() {
  var self = this;
  var filters = self.editingFilterList;
  $("#filter-name-list").empty();

  $.map(filters, function(filter) {
    $("<li/>").append(
      $("<a>", {
        text: filter.name,
        href: "#",
        click: function(obj) {
          self.selectedFilterNameElement = this;
          self.setCurrentFilterConfig(filter);
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
        var newFilter = self.getDefaultFilterConfig();
        newFilter.name = self.defaultFilterName;
        self.editingFilterList.push(newFilter);
        self.setCurrentFilterConfig(newFilter);
        self.resetFilterList();
      },
    })
  ).appendTo("#filter-name-list");

  resetDefaultFilterList(
    "summary", self.getValue('events.summary.default-filter-id'));
  resetDefaultFilterList(
    "events", self.getValue('events.default-filter-id'));

  function resetDefaultFilterList(type, defaultFilterId) {
    var elementId = "#" + type + "-default-filter-selector";

    if (!self.getFilterConfig(defaultFilterId, self.editingFilterList))
      defaultFilterId = 0;

    $(elementId).empty();
    $.map(filters, function(filter, i) {
      var option = $("<option/>", {
        text: filter.name,
      }).val(i).appendTo(elementId);

      if (defaultFilterId && defaultFilterId > 0) {
        if (defaultFilterId == filter.id)
          option.attr("selected", true);
      } else if (i === 0) {
        option.attr("selected", true);
      }
    });
  }
};

HatoholEventsViewConfig.prototype.setFilterCandidates = function(candidates) {
  var self = this;

  self.options.filterCandidates = candidates;
  self.setCurrentFilterConfig();
};

HatoholEventsViewConfig.prototype.setCurrentFilterConfig = function(filter) {
  var self = this;

  var candidates = self.options.filterCandidates;
  var serverKey, groupKey, hostKey;

  filter = filter || {};

  if (self.selectedFilterConfig)
    $.extend(self.selectedFilterConfig,
             self.getCurrentFilterConfig());

  $("#filter-name-entry").val(filter.name);
  $("#period-filter-setting").val(filter.days);

  // rebuild candidates of servers/hostgroups/hosts
  candidates.server = [];
  candidates.hostgroup = [];
  candidates.host = [];

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
    $("#enable-" + type + "-filter-selector").change();
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
      "[value=" + exclude + "]").prop('checked', true);

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

  self.selectedFilterConfig = filter;
};

HatoholEventsViewConfig.prototype.getDefaultFilterConfig = function() {
  return {
    name: gettext("ALL (31 days)"),
    days: 31,
    incident: {
      enable: false,
      selected: []
    },
    type: {
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

HatoholEventsViewConfig.prototype.getCurrentFilterConfig = function() {
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

HatoholEventsViewConfig.prototype.getFilterConfig = function(id, filterList) {
  var self = this, i;
  filterList = filterList || self.filterList;
  for (i = 0; i < filterList.length; i++) {
    if (filterList[i].id == id)
      return filterList[i];
  }
  return null;
};

HatoholEventsViewConfig.prototype.createFilter = function(filterConfig) {
  var conf = filterConfig;
  var filter = {}, selectHosts = [], excludeHosts = [], hostList;
  var secondsInDay = 60 * 60 * 24;
  var now = parseInt(Date.now() / 1000, 10);

  if (conf.days > 0)
    filter["beginTime"] = parseInt(now - secondsInDay * conf.days, 10);

  if (conf.incident.enable && conf.incident.selected.length > 0)
    filter["incidentStatuses"] = conf.incident.selected.join(",");

  if (conf.type.enable && conf.type.selected.length > 0)
    filter["types"] = conf.type.selected.join(",");

  if (conf.severity.enable && conf.severity.selected.length > 0)
    filter["severities"] = conf.severity.selected.join(",");

  if (conf.server.enable) {
    hostList = conf.server.exclude ? excludeHosts : selectHosts;
    $.map(conf.server.selected, function(id) {
      hostList.push({ serverId: id });
    });
  }

  if (conf.hostgroup.enable) {
    hostList = conf.hostgroup.exclude ? excludeHosts : selectHosts;
    $.map(conf.hostgroup.selected, function(item) {
      var pair = item.split(",", 2);
      hostList.push({ serverId: pair[0], hostgroupId: pair[1] });
    });
  }

  if (conf.host.enable) {
    hostList = conf.host.exclude ? excludeHosts : selectHosts;
    $.map(conf.host.selected, function(item) {
      var pair = item.split(",", 2);
      hostList.push({ serverId: pair[0], hostId: pair[1] });
    });
  }

  if (selectHosts.length > 0)
    filter["selectHosts"] = selectHosts;
  if (excludeHosts.length > 0)
    filter["excludeHosts"] = excludeHosts;

  return filter;
};

HatoholEventsViewConfig.prototype.getFilter = function(filterId) {
  var self = this;
  var filterConfig = self.getFilterConfig(filterId);
  filterConfig = filterConfig || self.getDefaultFilterConfig();
  return self.createFilter(filterConfig);
};

HatoholEventsViewConfig.prototype.showXHRError = function (XMLHttpRequest) {
  var errorMsg = "Error: " + XMLHttpRequest.status + ": " +
    XMLHttpRequest.statusText;
  hatoholErrorMsgBox(errorMsg);
};
