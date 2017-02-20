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
  self.servers = null;

  self.multiselectFilterTypes = [
    "incident", "type", "severity", "server", "hostgroup", "host"
  ];
  self.defaultFilterName = gettext("New filter");
  self.defaultConfig = {
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
  self.defaultFilterConfig = {
    name: self.defaultFilterName,
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

  self.config = $.extend(true, {}, self.defaultConfig);
  self.filterList = [$.extend(true, {}, self.defaultFilterConfig)];
  self.selectedFilterIndex = 0;

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
    for (var i=0; self.filterList.length > i; i++) {
      if (self.filterList[i].id == undefined)
        self.filterList.splice(i, 1);
    }
    self.selectedFilterIndex = 0;
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
    var value = parseInt($("#auto-reload-interval").val());

    if (value < minAutoReloadInterval) {
      value = minAutoReloadInterval;
    }
    if (value > maxAutoReloadInterval) {
      value = maxAutoReloadInterval;
    }
    if (isNaN(value)) {
      value = defaultAutoReloadInterval;
    }

    $("#auto-reload-interval").val(value);
    $("#auto-reload-interval-slider").slider("value", value);
  });

  $("#num-rows-per-page").change(function() {
    var value = parseInt($("#num-rows-per-page").val());

    if (value < minNumberOfRows) {
      value = minNumberOfRows;
    }
    if (value > maxNumberOfRows) {
      value = maxNumberOfRows;
    }
    if (isNaN(value)) {
      value = defaultNumberOfRows;
    }
    $("#num-rows-per-page").val(value);
  });

  $("#period-filter-setting").change(function() {
    var value = parseInt($("#period-filter-setting").val());

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
    $("#period-filter-setting").val(value);
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

    $('#' + selector + '-filter-selector').multiselect({
      right: "#" + selector + "-filter-selector-selected",
    });
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
    var tabId = $(".modal-body").find("div.active")[0].id;
    if (tabId == "view-config"){
      saveViewConfig();
    } else if (tabId == "filter-config"){
      saveFilterConfig();
    }else if (tabId == "default-filter-config"){
      saveDefaultFilterConfig();
    }
  });

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
  });

  $("#filter-name-list-delete-button").click(function () {
    if (!self.filterList[self.selectedFilterIndex].id)
      return;

    new HatoholConnector({
      pathPrefix: "",
      url: "/event-filters/" + self.filterList[self.selectedFilterIndex].id,
      request: "DELETE",
      replyCallback: function(reply, parser) {
        self.filterList.splice(self.selectedFilterIndex, 1);
        if (self.filterList.length == 0)
          self.filterList = [$.extend(true, {}, self.defaultFilterConfig)];
        self.selectedFilterIndex = 0;
        self.setCurrentFilterConfig();
        self.reset();
        if (self.options.savedCallback)
          self.options.savedCallback(self);
        showSucccessDialog(gettext("Successfully deleted"));
      },
      parseErrorCallback: function(reply, parser) {
        hatoholErrorMsgBoxForParser(reply, parser);
      },
    });
  });

  start();

  function saveViewConfig(){
    if ($('#column-selector-selected')[0].length < 1){
      showEmptyErrorDialog(["column"]);
      return;
    }

    $.extend(self.config, {
      'events.auto-reload.interval': $("#auto-reload-interval").val(),
      'events.num-rows-per-page': $("#num-rows-per-page").val(),
      'events.columns': buildSelectedColumns(),
    });

    self.store({
      items: self.config,
      successCallback: function(reply) {
        self.reset();
        showSucccessDialog();
      },
      connectErrorCallback: function(XMLHttpRequest, textStatus, errorThrown) {
        self.showXHRError(XMLHttpRequest);
      },
      completionCallback: function() {
        if (self.options.savedCallback)
          self.options.savedCallback(self);
      },
    });
  };

  function saveDefaultFilterConfig(){
    $.extend(self.config, {
      'events.summary.default-filter-id': $("#summary-default-filter-selector").val(),
      'events.default-filter-id': $("#events-default-filter-selector").val(),
    });

    self.store({
      items: self.config,
      successCallback: function(reply) {
        self.reset();
        showSucccessDialog();
      },
      connectErrorCallback: function(XMLHttpRequest, textStatus, errorThrown) {
        self.showXHRError(XMLHttpRequest);
      },
      completionCallback: function() {
        if (self.options.savedCallback)
          self.options.savedCallback(self);
      },
    });
  };

  function saveFilterConfig(){
    var emptyFilterList = [], floodFilterList = [];
    $.map(self.multiselectFilterTypes, function(selector) {
      if ($('#' + selector + '-filter-selector-selected')[0].length == 0 &&
          $('#enable-' + selector + '-filter-selector').prop("checked"))
        emptyFilterList.push(selector);

      if ($('#' + selector + '-filter-selector-selected')[0].length > 100 &&
          $('#enable-' + selector + '-filter-selector').prop("checked"))
        floodFilterList.push(selector);

    });
    if (emptyFilterList.length != 0) {
      showEmptyErrorDialog(emptyFilterList);
      return;
    }

    if (floodFilterList.length != 0) {
      showFloodErrorDialog(floodFilterList);
      return;
    }

    var filter = self.getCurrentFilterConfig();
    var path = "/event-filters/";
    if (filter.id)
      path += filter.id;

    new HatoholConnector({
      pathPrefix: "",
      url: path,
      request: filter.id ? "PUT" : "POST",
      data: JSON.stringify(filter),
      replyCallback: function(reply, parser) {
        self.filterList[self.selectedFilterIndex].id = reply.id;
        self.reset();
        showSucccessDialog();
        self.options.savedCallback(self);
      },
      parseErrorCallback: function(reply, parser) {
        hatoholErrorMsgBoxForParser(reply, parser);
      },
    });
  };

  function buildSelectedColumns() {
    var selected = $("#column-selector-selected option");
    var values = $.map(selected, function(option) { return $(option).val(); });
    return values.join(',');
  }

  function showEmptyErrorDialog(emptyFilterList) {
    var body = gettext("The following filter(s) empty.") + "</br>";
      for (var x = 0; x < emptyFilterList.length; x++) {
        body += " - " + self.filterTypeLabelMap[emptyFilterList[x]] + "</br>";
     }
    showErrorDialog(body);
  }

  function showFloodErrorDialog(floodFilterList) {
    var body = gettext("Is selecting over limit number of items in the following filter(s).") + "</br>";
      for (var x = 0; x < floodFilterList.length; x++) {
        body += " - " + self.filterTypeLabelMap[floodFilterList[x]] + "</br>";
     }
    showErrorDialog(body);
  }

  function showErrorDialog(body) {
    var label = gettext("Close");
    var footer = '<button type="button" class="btn btn-primary"';
    footer += 'id="close-button">' + label + '</button>';

    var errorDialog = new HatoholModal({
      "id": "filter-error-dialog",
      "title": gettext("Failed to save"),
      "body": body,
      "footer": $(footer),
    });
    $("#close-button").on("click", function() {
      errorDialog.close();
      $('#filter-error-dialog').on('hidden.bs.modal', function () {
        $('body').addClass('modal-open');
      });
    });
    errorDialog.show();
  }

  function showSucccessDialog(body){
    var successDialog = new HatoholModal({
      "id": "filter-success-dialog",
      "body": body || gettext("Successfully saved"),
    });
    successDialog.show();
    setTimeout(function(){
      successDialog.close();
      $('#filter-success-dialog').on('hidden.bs.modal', function () {
        $('body').addClass('modal-open');
      });
    }, 1500);
  }

  function quickHostFilter(word) {
    var setVisible = function(option) {
      var obj = $(option);
      var text = obj.text().toLowerCase();
      if (word.length === 0 || text.indexOf(word.toLowerCase()) >= 0)
        obj.show();
      else
        obj.hide();
    };
    $.map($("#host-filter-selector option"), setVisible);
    $.map($("#host-filter-selector-selected option"), setVisible);
  }

  function getFilterNameList(){
    var filterNameList = [];
    var anchors = $("#filter-name-list").children("li").find("a");
    for (var i=0; anchors.length - 1 > i; i++) {
      filterNameList.push(anchors[i].text);
    };
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

  function start() {
    var configDeferred = new $.Deferred();
    var filterDeferred = new $.Deferred();

    self.get({
      itemNames: Object.keys(self.defaultConfig),
      successCallback: function(config) {
        $.extend(self.config, config);
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
          self.filterList = [$.extend(true, {}, self.defaultFilterConfig)];
        self.reset();
        self.setCurrentFilterConfig();
      },
      parseErrorCallback: function(reply, parser) {
        hatoholErrorMsgBoxForParser(reply, parser);
      },
      completionCallback: function() {
        filterDeferred.resolve();
      },
    });

    $.when(configDeferred.promise(), filterDeferred.promise()).done(function() {
      if (self.options.loadedCallback)
        self.options.loadedCallback(self);
    });
  };
};

HatoholEventsViewConfig.prototype = Object.create(HatoholUserConfig.prototype);
HatoholEventsViewConfig.prototype.constructor = HatoholEventsViewConfig;

HatoholEventsViewConfig.prototype.getValue = function(key) {
  return this.findOrDefault(this.config, key, this.defaultConfig[key]);
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
  resetFilterList();
  resetDefaultFilterList(
    "summary", self.getValue('events.summary.default-filter-id'));
  resetDefaultFilterList(
    "events", self.getValue('events.default-filter-id'));
  self.setCurrentFilterConfig();

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

  function resetFilterList() {
    $("#filter-name-list").empty();

    if (self.filterList.length ==  0)
      self.filterList.push($.extend(true, {}, self.defaultFilterConfig));
    $.map(self.filterList, function(filter, i) {
      $("<li/>").append(
        $("<a>", {
          text: filter.name,
          href: "#",
          click: function(obj) {
            self.selectedFilterIndex = i;
            self.setCurrentFilterConfig();
            resetFilterList();
          },
        })
      ).appendTo("#filter-name-list");
    });

    for(var i = 0; i < self.filterList.length; i++) {
      if (self.filterList[i].id === undefined)
        return;
    }

    $("<li/>", {"class": "divider"}).appendTo("#filter-name-list");

    $("<li/>").append(
      $("<a>", {
        text: gettext("Add a new filter"),
        href: "#",
        click: function(obj) {
          var newFilter = $.extend(true, {}, self.defaultFilterConfig);
          newFilter.name = self.defaultFilterName;
          self.filterList.push(newFilter);
          self.selectedFilterIndex = self.filterList.length - 1;
          self.setCurrentFilterConfig();
          $("#filter-name-entry").change();
          self.filterList[self.filterList.length - 1].name =
            $("#filter-name-entry").val();
          $("#filter-name-entry").focus();
          $("#filter-name-entry").select();
          resetFilterList();
        },
      })
    ).appendTo("#filter-name-list");
  }

  function resetDefaultFilterList(type, defaultFilterId) {
    var elementId = $("#" + type + "-default-filter-selector").empty();

    if (!self.getFilterConfig(defaultFilterId).id)
      defaultFilterId = 0;

    $.map(self.filterList, function(filter, i) {
      var option = $("<option/>", {
        text: filter.name,
        value: filter.id
      }).appendTo(elementId);

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
};

HatoholEventsViewConfig.prototype.setCurrentFilterConfig = function() {
  var self = this;

  var candidates = self.options.filterCandidates;
  var serverKey, groupKey, hostKey;

  var filter = self.filterList[self.selectedFilterIndex];

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
};

HatoholEventsViewConfig.prototype.getCurrentFilterConfig = function() {
  var self = this;
  var filter = self.filterList[self.selectedFilterIndex];

  filter.name = $("#filter-name-entry").val();
  filter.days = $("#period-filter-setting").val();

  if (isNaN(filter.days))
    filter.days = 0;

  $.map(self.multiselectFilterTypes, function(type) {
    var config = {};
    config.enable = $("#enable-" + type +
                      "-filter-selector").prop("checked");

    var exclude = $("input:radio[name=" + type +
                    "-filter-select-options]:checked").val();
    if (exclude == "1")
      config.exclude = true;
    else if (exclude == "0")
      config.exclude = false;

    var selected = $("#" + type + "-filter-selector-selected option");
    config.selected = $.map(selected, function(option) {
      return $(option).val();
    });

    filter[type] = config;
  });

  return filter;
};

HatoholEventsViewConfig.prototype.setServers = function(servers) {
  this.servers = servers;
};

HatoholEventsViewConfig.prototype.getFilterConfig = function(filterId) {
  var self = this, i;
  for (i = 0; i < self.filterList.length; i++) {
    if (self.filterList[i].id == filterId)
      return self.filterList[i];
  }
  return $.extend(true, {}, self.defaultFilterConfig);
};

HatoholEventsViewConfig.prototype.getFilter = function(filterId) {
  var self = this;

  return createFilter(self.getFilterConfig(filterId));

  function createFilter(filterConfig) {
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
};
