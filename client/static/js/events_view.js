/*
 * Copyright (C) 2013-2016 Project Hatohol
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

var EventsView = function(userProfile, options) {
  var self = this;
  var params = deparam();
  self.options = options || {};
  self.userConfig = null;
  self.columnNames = [];
  self.reloadIntervalSeconds = 60;
  self.currentPage = 0;
  self.limitOfUnifiedId = 0;
  self.rawData = {};
  self.rawSummaryData = {};
  self.rawSeverityRankData = {};
  self.severityRanksMap = {};
  self.durations = {};
  self.baseQuery = {
    limit:            50,
    offset:           0,
    limitOfUnifiedId: 0,
    sortType:         "time",
    sortOrder:        hatohol.DATA_QUERY_OPTION_SORT_DESCENDING,
  };
  $.extend(self.baseQuery, getEventsQueryInURI());
  self.lastFilterId = null;
  self.lastQuickFilter = {};
  self.isFilteringOptionsUsed = false;
  self.showToggleAutoRefreshButton();
  self.setupToggleAutoRefreshButtonHandler(load, self.reloadIntervalSeconds);
  self.abbreviateDescriptionLength = 30;

  setupEventsTable();
  setupToggleFilter();
  setupToggleSidebar();

  if (self.options.disableTimeRangeFilter) {
   // Don't enable datetimepicker for tests.
  } else {
    setupTimeRangeFilter();
  }

  var eventPropertyChoices = {
    incident: [
      { value: "NONE",        label: pgettext("Incident", "NONE") },
      { value: "HOLD",        label: pgettext("Incident", "HOLD") },
      { value: "IN PROGRESS", label: pgettext("Incident", "IN PROGRESS") },
      { value: "DONE",        label: pgettext("Incident", "DONE") },
    ],
    status: [
      { value: "0", label: gettext('OK') },
      { value: "1", label: gettext('Problem') },
      { value: "2", label: gettext('Unknown') },
    ],
    type: [
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
  };
  var defaultIncidentLabelMap = buildLabelMap(eventPropertyChoices.incident);
  var incidentLabelMap = defaultIncidentLabelMap;

  var columnDefinitions = {
    "monitoringServerName": {
      header: gettext("Monitoring Server"),
      body: renderTableDataMonitoringServer,
    },
    "hostName": {
      header: gettext("Host"),
      body: renderTableDataHostName,
    },
    "eventId": {
      header: gettext("Event ID"),
      body: renderTableDataEventId,
    },
    "type": {
      header: gettext("Status"),
      body: renderTableDataEventType,
    },
    "status": {
      header: gettext("Current trigger status"),
      body: renderTableDataTriggerStatus,
    },
    "severity": {
      header: gettext("Severity"),
      body: renderTableDataEventSeverity,
    },
    "time": {
      header: gettext("Period"),
      body: renderTableDataEventTime,
    },
    "description": {
      header: gettext("Brief"),
      body: renderTableDataEventDescription,
    },
    "duration": {
      header: gettext("Duration"),
      body: renderTableDataEventDuration,
    },
    "incidentStatus": {
      header: gettext("Handling"),
      body: renderTableDataIncidentStatus,
    },
    "incidentPriority": {
      header: gettext("Priority"),
      body: renderTableDataIncidentPriority,
    },
    "incidentAssignee": {
      header: gettext("Assignee"),
      body: renderTableDataIncidentAssignee,
    },
    "incidentDoneRatio": {
      header: gettext("% Done"),
      body: renderTableDataIncidentDoneRatio,
    },
  };

  // call the constructor of the super class
  HatoholMonitoringView.apply(this, [userProfile]);

  self.pager = new HatoholEventPager();
  start();

  //
  // Private functions
  //
  function buildLabelMap(array) {
    var labelMap = {};
    $.map(array, function(obj, i) {
      if (obj.value)
        labelMap[obj.value] = obj.label;
    });
    return labelMap;
  }

  function start() {
    $.when(loadUserConfig(), loadSeverityRank(), loadCustomIncidentStatus()).done(function() {
      self.userConfig.setFilterCandidates(eventPropertyChoices);
      load();
    }).fail(function() {
      hatoholInfoMsgBox(gettext("Failed to get the configuration!"));
      load(); // Ensure to work with the default config
    });
  }

  function loadUserConfig() {
    var deferred = new $.Deferred();
    self.userConfig = new HatoholEventsViewConfig({
      columnDefinitions: columnDefinitions,
      filterCandidates: eventPropertyChoices,
      loadedCallback: function(config) {
        applyConfig(config);
        updatePager();
        setupFilterValues();
        setupCallbacks();
        deferred.resolve();
      },
      savedCallback: function(config) {
        applyConfig(config);
        load();
      },
    });
    return deferred.promise();
  }

  function loadSeverityRank() {
    var deferred = new $.Deferred();
    new HatoholConnector({
      url: "/severity-rank",
      request: "GET",
      replyCallback: function(reply, parser) {
        var i, severityRanks, rank;
        var choices = eventPropertyChoices.severity;
        self.rawSeverityRankData = reply;
        self.severityRanksMap = {};
        severityRanks = self.rawSeverityRankData["SeverityRanks"];
        if (severityRanks) {
          for (i = 0; i < severityRanks.length; i++) {
            self.severityRanksMap[severityRanks[i].status] = severityRanks[i];
          }
          for (i = 0; i < choices.length; i++) {
            rank = self.severityRanksMap[choices[i].value];
            if (rank && rank.label)
              choices[i].label = rank.label;
          }
        }
        deferred.resolve();
      },
      parseErrorCallback: function() {
        deferred.reject();
      },
      connectErrorCallback: function() {
        deferred.reject();
      },
    });
    return deferred.promise();
  }

  function loadCustomIncidentStatus() {
    var deferred = new $.Deferred();
    new HatoholConnector({
      url: "/custom-incident-status",
      request: "GET",
      replyCallback: function(reply, parser) {
        var i, incidentStatuses, status, label;

        incidentStatuses = reply["CustomIncidentStatuses"];

        if (!incidentStatuses || incidentStatuses.length <= 0) {
          deferred.resolve();
          return;
        }

        eventPropertyChoices.incident = [];
        for (i = 0; i < incidentStatuses.length; i++) {
          status = incidentStatuses[i];
          if (status.label === "") {
            if (defaultIncidentLabelMap[status.code]) {
              label = defaultIncidentLabelMap[status.code];
            } else {
              label = null;
            }
          } else {
            label = status.label;
          }

          if (label) {
            eventPropertyChoices.incident.push({
              value: status.code,
              label: label,
            });
          }
          incidentLabelMap = buildLabelMap(eventPropertyChoices.incident);
        }

        deferred.resolve();
      },
      parseErrorCallback: function() {
        deferred.reject();
      },
      connectErrorCallback: function() {
        deferred.reject();
      },
    });
    return deferred.promise();
  }

  function applyConfig(config) {
    var defaultFilterId = config.getValue('events.default-filter-id');
    var defaultSummaryFilterId = config.getValue('events.summary.default-filter-id');
    self.reloadIntervalSeconds = config.getValue('events.auto-reload.interval');
    self.baseQuery.limit = config.getValue('events.num-rows-per-page');
    self.baseQuery.sortType = config.getValue('events.sort.type');
    self.baseQuery.sortOrder = config.getValue('events.sort.order');
    self.columnNames = config.getValue('events.columns').split(',');

    // Reset filter menu
    self.lastFilterId = defaultFilterId;
    $("#select-filter").empty();
    $("#select-summary-filter").empty();
    $.map(config.filterList, function(filter) {
      var option = $("<option/>", {
        text: filter.name,
      }).val(filter.id).appendTo("#select-filter");
      if (filter.id == defaultFilterId)
        option.attr("selected", true);

      option = $("<option/>", {
        text: filter.name,
      }).val(filter.id).appendTo("#select-summary-filter");
      if (filter.id == defaultSummaryFilterId)
        option.attr("selected", true);
    });
    setupFilterValues();
    resetQuickFilter();

    // summary
    if (config.config["events.show-sidebar"] == "false") {
      $("#event-table-area").removeClass("col-md-10");
      $("#event-table-area").addClass("col-md-12");
      $("#sidebar-left-glyph").toggle();
      $("#sidebar-right-glyph").toggle();
    } else {
      $("#SummarySidebar").show();
    }
  }

  function updatePager() {
    self.pager.update({
      currentPage: self.currentPage,
      numRecords: self.rawData ? self.rawData["numberOfEvents"] : -1,
      numRecordsPerPage: self.baseQuery.limit,
      selectPageCallback: function(page) {
        if (self.pager.numRecordsPerPage != self.baseQuery.limit)
          self.baseQuery.limit = self.pager.numRecordsPerPage;
        load({ page: page });
      }
    });
  }

  function updateFilteringResult() {
    var numEvents = self.rawData["events"].length;
    var title;
    var filteringOpts = $("#filtering-options-brief");
    if (!self.isFilteringOptionsUsed) {
        filteringOpts.hide();
        title = gettext("All Events");
    } else {
        filteringOpts.show();
        $("#filtering-options-brief-line").text(getBriefOfFilteringOptions());
        title = gettext("Filtering Results");
    }
    title += " (" + numEvents + ")";
    $("#controller-container-title").text(title);
  }

  function appendFilteringTimeRange(elementId, briefObj) {
    var value = $(elementId).val();
    if (!value)
      value = $(elementId).attr("placeholder");
    briefObj["brief"] += value;
  }

  function appendFilteringValue(elementId, briefObj) {
    var value = $(elementId + " option:selected").text();
    if (value == "---------")
      return;
    briefObj["brief"] += ", ";
    briefObj["brief"] += value;
  }

  function getBriefOfFilteringOptions() {
    var s = {"brief":""};
    appendFilteringTimeRange("#begin-time", s);
    s["brief"] += " - ";
    appendFilteringTimeRange("#end-time", s);

    appendFilteringValue("#select-incident", s);
    appendFilteringValue("#select-type", s);
    appendFilteringValue("#select-severity", s);
    appendFilteringValue("#select-server", s);
    appendFilteringValue("#select-host-group", s);
    appendFilteringValue("#select-host", s);

    appendFilteringValue("#select-filter", s);
    return s["brief"];
  }

  function showXHRError(XMLHttpRequest) {
    var errorMsg = "Error: " + XMLHttpRequest.status + ": " +
      XMLHttpRequest.statusText;
    hatoholErrorMsgBox(errorMsg);
  }

  function getEventsQueryInURI() {
    var knownKeys = [
      "serverId", "hostgroupId", "hostId",
      "limit", "offset", "limitOfUnifiedId",
      "sortType", "sortOrder",
      "type", "minimumSeverity", "status", "triggerId",
    ];
    var i, allParams = deparam(), query = {};
    for (i = 0; i < knownKeys.length; i++) {
      if (knownKeys[i] in allParams)
        query[knownKeys[i]] = allParams[knownKeys[i]];
    }
    return query;
  }

  function getQuickFilter() {
    var query = {};

    if ($("#select-incident").val())
      query.incidentStatuses = $("#select-incident").val();

    if ($("#select-type").val())
      query.type = $("#select-type").val();

    if ($("#select-severity").val())
      query.minimumSeverity =  $("#select-severity").val();

    var beginTime, endTime;
    if ($('#begin-time').val()) {
      beginTime = new Date($('#begin-time').val());
      query.beginTime = parseInt(beginTime.getTime() / 1000);
    }
    if ($('#end-time').val()) {
      endTime = new Date($('#end-time').val());
      query.endTime = parseInt(endTime.getTime() / 1000);
    }

    $.extend(query, self.getHostFilterQuery());

    return query;
  }

  function getQuery(options) {
    var query = {}, baseFilter;

    options = options || {};

    if (!options.page) {
      self.limitOfUnifiedId = 0;
    } else {
      if (!self.limitOfUnifiedId)
        self.limitOfUnifiedId = self.rawData.lastUnifiedEventId;
    }

    if (!self.lastFilterId)
      self.lastFilterId = self.userConfig.getValue("events.default-filter-id");
    if (options.applyFilter) {
      self.lastFilterId = $("#select-filter").val();
      self.lastQuickFilter = getQuickFilter();
    }
    baseFilter = self.userConfig.getFilter(self.lastFilterId);

    $.extend(query, self.baseQuery, baseFilter, self.lastQuickFilter, {
      offset:           self.baseQuery.limit * self.currentPage,
      limit:            self.baseQuery.limit,
      limitOfUnifiedId: self.limitOfUnifiedId,
    });

    // TODO: Should set it by caller
    var beginTime = new Date(baseFilter.beginTime * 1000);
    var beginTimeString = formatDateTimeWithZeroSecond(beginTime);
    $("#begin-time").attr("placeholder", beginTimeString);

    return 'events?' + $.param(query);
  }

  function getSummaryQuery() {
    var query = {}, baseFilterId, baseFilter;

    baseFilterId = $("#select-summary-filter").val();
    baseFilter = self.userConfig.getFilter(baseFilterId);
    $.extend(query, baseFilter);

    return 'summary/important-event?' + $.param(query);
  }

  function load(options) {
    var query;
    options = options || {};
    self.displayUpdateTime();
    setLoading(true);
    if (!isNaN(options.page)) {
      self.currentPage = options.page;
      self.disableAutoRefresh();
    } else {
      options.page = 0;
      self.currentPage = 0;
    }
    self.startConnection(getQuery(options), updateCore);
    var summaryShown = $("#event-table-area").hasClass("col-md-10");
    if (summaryShown)
      self.startConnection(getSummaryQuery(), updateSummary);
    $(document.body).scrollTop(0);
  }

  function resetTimeRangeFilter() {
    $("#begin-time").next(".clear-button").trigger('click');
    $("#end-time").next(".clear-button").trigger('click');
  }

  function resetQuickFilter() {
    $("#select-incident").val("");
    $("#select-type").val("");
    $("#select-severity").val("");
    $("#select-server").val("");
    $("#select-host-group").val("");
    $("#select-host").val("");
  }

  function removeUnselectableFilterCandidates(filterConfig, type, serverId) {
    var conf = filterConfig;
    var useAll = (!conf[type].enable || conf[type].selected.length <= 0);
    var selected = {};
    var shouldShow = function(serverId, id, selected, exclude) {
      var unifiedId = id;
      if (serverId)
        unifiedId = "" + serverId + "," + id;
      if (!id)
        return true;
      if (!exclude && selected[unifiedId])
        return true;
      if (exclude && !selected[unifiedId])
        return true;
      return false;
    };
    var elementId = "#select-" + type;

    if (type == "hostgroup")
      elementId = "#select-host-group";

    if (conf[type].enable) {
      $.map(conf[type].selected, function(id) {
        selected[id] = true;
      });
      $(elementId + " option").map(function() {
        var id = $(this).val();
        var exclude = conf[type].exclude;
        if (!useAll && !shouldShow(serverId, id, selected, exclude))
          $(this).remove();
      });
    }
  }

  function resetEventPropertyFilter(filterConfig, type) {
    var conf = filterConfig;
    var candidates = eventPropertyChoices[type];
    var option;
    var selectedCandidates = {};
    var useAllItems = (!conf[type].enable || conf[type].selected.length <= 0);
    var currentId = $("#select-" + type).val();

    $.map(conf[type].selected, function(selected) {
      selectedCandidates[selected] = true;
    });

    $("#select-" + type).empty();

    option = $("<option/>", {
      text: "---------",
      value: "",
    }).appendTo("#select-" + type);

    $.map(candidates, function(candidate) {
      var option;

      if (useAllItems || selectedCandidates[candidate.value]) {
        option = $("<option/>", {
          text: candidate.label,
          value: candidate.value
        }).appendTo("#select-" + type);
      }
    });

    $("#select-" + type).val(currentId);
  }

  function setupChangeIncidentCandidates() {
    var selector = "#change-incident";
    var candidates = eventPropertyChoices.incident;
    var currentId = $(selector).val();

    $(selector).empty();

    $("<option/>", {
      text: "---------",
      value: "",
    }).appendTo(selector);

    $.map(candidates, function(aCandidate) {
      $("<option/>", {
        text: aCandidate.label,
        value: aCandidate.value,
      }).appendTo(selector);
    });

    $(selector).val(currentId);
  }

  function setupFilterValues(servers, query) {
    var serverId = $("#select-server").val();
    var filterId = $("#select-filter").val();
    var filterConfig = self.userConfig.getFilterConfig(filterId);
    filterConfig = filterConfig || self.userConfig.getDefaultFilterConfig();

    if (!servers && self.rawData && self.rawData.servers)
      servers = self.rawData.servers;

    if (!query)
      query = self.lastQuickFilter ? self.lastQuickFilter : self.baseQuery;

    self.setupHostFilters(servers, query);

    resetEventPropertyFilter(filterConfig, "type");
    resetEventPropertyFilter(filterConfig, "severity");
    resetEventPropertyFilter(filterConfig, "incident");
    setupChangeIncidentCandidates();
    setupChangeIncidentMenu();

    removeUnselectableFilterCandidates(filterConfig, "server");
    if (serverId) {
      removeUnselectableFilterCandidates(filterConfig, "hostgroup", serverId);
      removeUnselectableFilterCandidates(filterConfig, "host", serverId);
    }

    if ("minimumSeverity" in query)
      $("#select-severity").val(query.minimumSeverity);
    if ("type" in query)
      $("#select-type").val(query.type);
  }

  function setupHandlingMenu() {
    var trackers = self.rawData.incidentTrackers;
    var enableIncident = self.rawData["haveIncident"];
    var hasIncidentTypeHatohol = false;
    var hasIncidentTypeOthers = false;

    if (!self.rawData["haveIncident"]) {
      $("#select-incident-container").hide();
      $("#change-incident-container").hide();
      fixupEventsTableHeight();
      return;
    }

    if (typeof trackers == "object") {
      $.map(trackers, function(tracker, key) {
        switch (tracker.type) {
        case hatohol.INCIDENT_TRACKER_HATOHOL:
          hasIncidentTypeHatohol = true;
          break;
        case hatohol.INCIDENT_TRACKER_REDMINE:
          hasIncidentTypeOthers = true;
          break;
        default:
          hasIncidentTypeOthers = true;
          break;
        }
      });
    }

    if (hasIncidentTypeHatohol && !hasIncidentTypeOthers) {
      $("#select-incident-container").show();
      $("#change-incident-container").show();
      $("#IncidentTypeHatoholNotAssignedEventLabel").show();
      $("#IncidentTypeHatoholImportantEventLabel").show();
      $("#IncidentTypeHatoholImportantEventProgress").show();
      fixupEventsTableHeight();
    } else {
      $("#select-incident-container").hide();
      $("#change-incident-container").hide();
      $("#IncidentTypeHatoholNotAssignedEventLabel").hide();
      $("#IncidentTypeHatoholImportantEventLabel").hide();
      $("#IncidentTypeHatoholImportantEventProgress").hide();
      fixupEventsTableHeight();
    }
  }

  function setupCallbacks() {
    $('#select-summary-filter').change(function() {
      load();
    });

    $('#select-server').change(function() {
      setupFilterValues(undefined, {});
    });

    $('button.latest-button').click(function() {
      $("#end-time").val("");
      $("#end-time").next(".clear-button").hide();
      load({ applyFilter: true });
    });

    $("#change-incident").change(function() {
      updateIncidentStatus();
    });

    $('button.reset-apply-all-filter').click(function() {
      resetTimeRangeFilter();
      resetQuickFilter();
      load({ applyFilter: true });
      self.isFilteringOptionsUsed = false;
    });

    $('button.btn-apply-all-filter').click(function() {
      load({ applyFilter: true });
      self.isFilteringOptionsUsed = true;
    });

    $("#select-filter").change(function() {
      setupFilterValues();
      resetQuickFilter();
    });

    $("#toggle-abbreviating-event-descriptions").attr("checked", false);
    $("#toggle-abbreviating-event-descriptions").change(function() {
      load();
    });
  }

  function updateIncidentStatus() {
    var status = $("#change-incident").val();
    var unifiedId, trackerId;
    var incidents = $(".incident.selected");
    var promise, promises = [], errors = [];
    var errorMessage;
    var selectedTrackerId;
    var i;

    if (!status)
      return;

    // Select an incident tracker to post new incidents
    for (i = 0; i < incidents.length; i++) {
      trackerId = parseInt(incidents[i].getAttribute("data-tracker-id"));
      if (isNaN(trackerId)) {
        selectedTrackerId = chooseIncidentTracker();
        if (!selectedTrackerId) {
          errorMessage = gettext("There is no valid incident tracker to post!");
          hatoholErrorMsgBox(errorMessage);
          return;
        }
        break;
      }
    }

    // update existing incidents and post new incidents
    for (i = 0; i < incidents.length; i++) {
      unifiedId = parseInt(incidents[i].getAttribute("data-unified-id"));
      trackerId = parseInt(incidents[i].getAttribute("data-tracker-id"));
      if (trackerId > 0) {
        promise = applyIncidentStatus(unifiedId, status, errors);
      } else {
        promise = addIncident(unifiedId, selectedTrackerId, status, errors);
      }
      promises.push(promise);
    }

    if (promises.length > 0) {
      hatoholInfoMsgBox(gettext("Appling the handling..."));
      $.when.apply($, promises).done(function() {
        if (errors.length === 0) {
          hatoholInfoMsgBox(gettext("Successfully updated."));
        } else {
          errorMessage = gettext("Failed to update handling");
          hatoholErrorMsgBox(errorMessage, { optionMessages: errors });
        }
        $("#change-incident").val("");
        $('.incident.selectable').removeClass("selected");
        setupChangeIncidentMenu();
        load();
      });
    }
  }

  function chooseIncidentTracker() {
    var trackers = self.rawData.incidentTrackers;
    var ids = Object.keys(trackers), id;

    // TODO: show a selection dialog when plural trackers exist

    for (var i = 0; i < ids.length; i++) {
      id = ids[i];
      if (trackers[id].type == hatohol.INCIDENT_TRACKER_HATOHOL) {
        return id;
      }
    }
    return undefined;
  }

  function addIncident(eventId, trackerId, status, errors) {
    var deferred = new $.Deferred();
    new HatoholConnector({
      url: "/incident",
      request: "POST",
      data: {
	  unifiedEventId: eventId,
	  incidentTrackerId: trackerId,
      },
      replyCallback: function() {
        var promise = applyIncidentStatus(eventId, status, errors);
        $.when(promise).done(function() {
          deferred.resolve();
        });
      },
      parseErrorCallback: function(reply, parser)  {
        var message = parser.getMessage();
        if (!message) {
          message =
            gettext("An unknown error occured on creating an incident for the event: ") +
            eventId;
        }
        if (parser.optionMessages)
          message += " " + parser.optionMessages;
        errors.push(message);
        deferred.resolve();
      },
      connectErrorCallback: function() {
        var message =
          gettext("Failed to connect to Hatohol server on posting an incident for the event: ") +
          eventId;
        errors.push(message);
        deferred.resolve();
      },
    });
    return deferred.promise();
  }

  function applyIncidentStatus(incidentId, status, errors) {
    var deferred = new $.Deferred();
    var url = "/incident";
    url += "/" + incidentId;
    new HatoholConnector({
      url: url,
      request: "PUT",
      data: { status: status },
      replyCallback: function() {
        // nothing to do
      },
      parseErrorCallback: function(reply, parser)  {
        var message = parser.getMessage();
        if (!message) {
          message =
            gettext("An unknown error occured on changing handling of an event with ID: ") +
            incidentId;
        }
        if (parser.optionMessages)
          message += " " + parser.optionMessages;
        errors.push(message);
      },
      connectErrorCallback: function() {
        /* Permit a long line for I18N message....*/
        /*jshint ignore:start*/
        var message =
          gettext("Failed to connect to Hatohol server on changing handling of an event with ID: ") +
          eventId;
        /*jshint ignore:end*/
        errors.push(message);
      },
      completionCallback: function() {
        deferred.resolve();
      },
    });
    return deferred.promise();
  }

  function replyCallback(reply, parser) {
    if (self.incidentTracker)
      hatoholInfoMsgBox(gettext("Successfully updated."));
    else
      hatoholInfoMsgBox(gettext("Successfully created."));

    if (self.succeededCallback)
      self.succeededCallback();
  }

  function formatDateTimeWithZeroSecond(d) {
    var t = "" + d.getFullYear() + "/";
    t += padDigit((d.getMonth() + 1), 2);
    t += "/";
    t += padDigit(d.getDate(), 2);
    t += " ";
    t += padDigit(d.getHours(), 2);
    t += ":";
    t += padDigit(d.getMinutes(), 2);
    t += ":00";
    return t;
  }

  function setupTimeRangeFilter() {
    $('#begin-time').datetimepicker({
      format: 'Y/m/d H:i:s',
      closeOnDateSelect: true,
      onSelectDate: function(currentTime, $input) {
        $('#begin-time').val(formatDateTimeWithZeroSecond(currentTime));
      },
      onSelectTime: function(currentTime, $input) {
        $('#begin-time').val(formatDateTimeWithZeroSecond(currentTime));
      },
    });

    $('#end-time').datetimepicker({
      format: 'Y/m/d H:i:s',
      closeOnDateSelect: true,
      onDateTime: function(currentTime, $input) {
        $('#end-time').val(formatDateTimeWithZeroSecond(currentTime));
      },
      onSelectTime: function(currentTime, $input) {
        $('#end-time').val(formatDateTimeWithZeroSecond(currentTime));
      },
    });

    $(".filter-time-range").change(function () {
      var input = $(this);
      input.next('span').toggle(!!(input.val()));
    });
    $(".clear-button").click(function(){
      $(this).prev('input').val('');
      $(this).hide();
    });
  }

  function fixupEventsTableHeight() {
    var offset = $(".event-table-content").offset();
    var height = $(window).height() - offset.top - $("#events-pager").height();
    $(".event-table-content").height(height);
  }

  function setupEventsTable() {
    fixupEventsTableHeight();
    $(window).resize(function () {
      fixupEventsTableHeight();
    });
  }

  function setupToggleFilter() {
    $("#hideDiv").hide();
    $('#hide').click(function(){
      $("#hideDiv").slideToggle(function() {
        fixupEventsTableHeight();
      });
      $("#filter-right-glyph").toggle();
      $("#filter-down-glyph").toggle();
    });
  }

  function setupToggleSidebar() {
    $("#toggle-sidebar").show();
    $("#toggle-sidebar").click(function(){
      $("#SummarySidebar").toggle();
      $("#event-table-area").toggleClass("col-md-12");
      $("#event-table-area").toggleClass("col-md-10");
      $("#sidebar-left-glyph").toggle();
      $("#sidebar-right-glyph").toggle();
      self.userConfig.saveValue("events.show-sidebar",
                                $("#event-table-area").hasClass("col-md-10"));
      var summaryShown = $("#event-table-area").hasClass("col-md-10");
      if (summaryShown) {
        self.startConnection(getSummaryQuery(), updateSummary);
      }
    });
  }

  function setLoading(loading) {
    if (loading) {
      $("#begin-time").attr("disabled", "disabled");
      $("#end-time").attr("disabled", "disabled");
      $("#select-incident").attr("disabled", "disabled");
      $("#select-severity").attr("disabled", "disabled");
      $("#select-type").attr("disabled", "disabled");
      $("#select-server").attr("disabled", "disabled");
      $("#select-host").attr("disabled", "disabled");
      $("#select-filter").attr("disabled", "disabled");
      $(".latest-button").attr("disabled", "disabled");
    } else {
      $("#begin-time").removeAttr("disabled");
      $("#end-time").removeAttr("disabled");
      $("#select-incident").removeAttr("disabled");
      $("#select-severity").removeAttr("disabled");
      $("#select-type").removeAttr("disabled");
      $("#select-server").removeAttr("disabled");
      if ($("#select-host option").length > 1)
        $("#select-host").removeAttr("disabled");
      $("#select-filter").removeAttr("disabled");
      $(".latest-button").removeAttr("disabled");
    }
  }

  function parseData(replyData) {
    // The structur of durations:
    // {
    //   serverId1: {
    //     triggerId1: {
    //       clock1: duration1,
    //       clock2: duration2,
    //       ...
    //     },
    //     triggerId2: ...
    //   },
    //   serverId2: ...
    // }

    var durations = {};
    var serverId, triggerId;
    var x, event, now, times, durationsForTrigger;

    // extract times from raw data
    for (x = 0; x < replyData["events"].length; ++x) {
      event = replyData["events"][x];
      serverId = event["serverId"];
      triggerId = event["triggerId"];

      if (!durations[serverId])
        durations[serverId] = {};
      if (!durations[serverId][triggerId])
        durations[serverId][triggerId] = [];

      durations[serverId][triggerId].push(event["time"]);
    }

    // create durations maps and replace times arrays with them
    for (serverId in durations) {
      for (triggerId in durations[serverId]) {
        times = durations[serverId][triggerId].uniq().sort();
        durationsForTrigger = {};
        for (x = 0; x < times.length; ++x) {
          if (x == times.length - 1) {
            now = parseInt((new Date()).getTime() / 1000);
            durationsForTrigger[times[x]] = now - Number(times[x]);
          } else {
            durationsForTrigger[times[x]] = Number(times[x + 1]) - Number(times[x]);
          }
        }
        durations[serverId][triggerId] = durationsForTrigger;
      }
    }

    return durations;
  }

  function isSelfMonitoringHost(hostId) {
    return  hostId == "__SELF_MONITOR";
  }

  function abbreviateDescription(description) {
    if (description.length > self.abbreviateDescriptionLength)
      return description.substr(0, self.abbreviateDescriptionLength) + " ...";
    else
      return description;
  }

  function getEventDescription(event) {
    var extendedInfo, name;

    try {
      extendedInfo = JSON.parse(event["extendedInfo"]);
      name = extendedInfo["expandedDescription"];
    } catch(e) {
    }

    if ($("#toggle-abbreviating-event-descriptions").is(":checked")) {
      return name ? abbreviateDescription(name) : abbreviateDescription(event["brief"]);
    } else {
      return name ? name : event["brief"];
    }
  }

  function getIncident(event) {
    if (!self.rawData["haveIncident"])
      return null;
    else
      return event["incident"];
  }

  function getSeverityClass(event) {
    var type = event["type"];
    var severity = event["severity"];
    var severityClass = "severity";

    if (type == hatohol.EVENT_TYPE_BAD)
      return "severity" + escapeHTML(severity);
    else
      return "";
  }

  function renderTableDataMonitoringServer(event, server) {
    var html = "";
    var serverId = event["serverId"];
    var serverURL = getServerLocation(server);
    var nickName = getNickName(server, serverId);

    html += "<td class='" + getSeverityClass(event) + "'>";
    if (serverURL) {
      html += "<a href='" + serverURL + "' target='_blank'>" +
        escapeHTML(nickName) + "</a></td>";
    } else {
      html += escapeHTML(nickName)+ "</td>";
    }

    return html;
  }

  function renderTableDataEventId(event, server) {
    return "<td class='" + getSeverityClass(event) + "'>" +
      escapeHTML(event["eventId"]) + "</td>";
  }

  function renderTableDataEventTime(event, server) {
    var html = "";
    var serverURL = getServerLocation(server);
    var hostId = event["hostId"];
    var triggerId = event["triggerId"];
    var eventId = event["eventId"];
    var clock = event["time"];

    html += "<td class='" + getSeverityClass(event) + "'>";
    if (serverURL && serverURL.indexOf("zabbix") >= 1 &&
	!isSelfMonitoringHost(hostId)) {
      html +=
      "<a href='" + serverURL + "tr_events.php?&triggerid=" +
        triggerId + "&eventid=" + eventId +
        "' target='_blank'>" + escapeHTML(formatDate(clock)) +
        "</a></td>";
    } else {
      html += formatDate(clock) + "</td>";
    }

    return html;
  }

  function renderTableDataHostName(event, server) {
    var html = "";
    var hostId = event["hostId"];
    var serverURL = getServerLocation(server);
    var hostName = getHostName(server, hostId);

    // TODO: Should be built by plugins
    html += "<td class='" + getSeverityClass(event) + "'>";
    if (serverURL && serverURL.indexOf("zabbix") >= 0 &&
        !isSelfMonitoringHost(hostId)) {
      html += "<a href='" + serverURL + "latest.php?&hostid=" +
              hostId + "' target='_blank'>" + escapeHTML(hostName) +
              "</a></td>";
    } else if (serverURL && serverURL.indexOf("nagios") >= 0 &&
               !isSelfMonitoringHost(hostId)) {
      html += "<a href='" + serverURL + "cgi-bin/status.cgi?host=" +
        hostName + "' target='_blank'>" + escapeHTML(hostName) + "</a></td>";
    } else {
      html += escapeHTML(hostName) + "</td>";
    }

    return html;
  }

  function renderTableDataEventDescription(event, server) {
    var description = getEventDescription(event);

    return "<td class='" + getSeverityClass(event) +
      "' title='" + escapeHTML(description) + "'>" +
      escapeHTML(description) + "</td>";
  }

  function renderTableDataEventType(event, server) {
    var type = event["type"];
    var typeClass = "event-type" + type;
    var typeIcon = {"0":"glyphicon-ok-sign","1":"glyphicon-remove-sign","2":"glyphicon-question-sign","3":"glyphicon-info-sign"}[type]||"glyphicon-question-sign";

    return "<td class='" + getSeverityClass(event) + " " + typeClass + "'>" +
      "<span class=\"eventStatus glyphicon "+typeIcon+"\"></span> "+
      eventPropertyChoices.type[Number(type)].label + "</td>";
  }

  function renderTableDataTriggerStatus(event, server) {
    var status = event["status"];
    var statusClass = "status" + status;

    return "<td class='" + getSeverityClass(event) + " " + statusClass + "'>" +
      eventPropertyChoices.status[Number(status)].label + "</td>";
  }

  function getSeverityLabel(event) {
    var severity = event["severity"];
    return eventPropertyChoices.severity[Number(severity)].label;
  }

  function getIncidentStatusLabel(event) {
    var incident = event["incident"];

    if (incidentLabelMap[incident.status])
      return incidentLabelMap[incident.status];
    if (defaultIncidentLabelMap[incident.status])
      return defaultIncidentLabelMap[incident.status];
    return incident.status;
  }

  function renderTableDataEventSeverity(event, server) {
    var severity = event["severity"];

    return "<td class='" + getSeverityClass(event) + "'>" +
      getSeverityLabel(event) + "</td>";
  }

  function renderTableDataEventDuration(event, server) {
    var serverId = event["serverId"];
    var triggerId  = event["triggerId"];
    var clock = event["time"];
    var duration = self.durations[serverId][triggerId][clock];

    return "<td class='" + getSeverityClass(event) + "'>" +
      formatSecond(duration) + "</td>";
  }

  function renderTableDataIncidentStatus(event, server) {
    var html = "", incident = getIncident(event);
    var unifiedId = event["unifiedId"], trackerId;

    html += "<td class='selectable incident " + getSeverityClass(event) + "'";
    html += " data-unified-id='" + unifiedId + "'";
    if (incident) {
      trackerId = incident["trackerId"];
      if (trackerId > 0)
        html += " data-tracker-id='" + trackerId + "'";
      else
        html += " data-tracker-id=''";
    }
    html += " style='display:none;'>";

    if (!incident)
      return html + "</td>";

    if (!incident.location)
      return html + getIncidentStatusLabel(event) + "</td>";

    html += "<a href='" + escapeHTML(incident.location) + "' target='_blank'>";
    html += getIncidentStatusLabel(event) + "</a>";
    html += "</td>";

    return html;
  }

  function renderTableDataIncidentPriority(event, server) {
    var html = "", incident = getIncident(event);

    html += "<td class='incident " + getSeverityClass(event) + "'";
    html += " style='display:none;'>";

    if (!incident)
      return html + "</td>";

    html += escapeHTML(incident.priority);
    html += "</td>";

    return html;
  }

  function renderTableDataIncidentAssignee(event, server) {
    var html = "", incident = getIncident(event);

    html += "<td class='incident " + getSeverityClass(event) + "'";
    html += " style='display:none;'>";

    if (!incident)
      return html + "</td>";

    html += escapeHTML(incident.assignee);
    html += "</td>";

    return html;
  }

  function renderTableDataIncidentDoneRatio(event, server) {
    var html = "", incident = getIncident(event);

    html += "<td class='incident " + getSeverityClass(event) + "'";
    html += " style='display:none;'>";

    if (!incident)
      return html + "</td>";

    if (incident.status)
      html += escapeHTML(incident.doneRatio) + "%";
    html += "</td>";

    return html;
  }

  function drawTableHeader() {
    var i, definition, columnName, isIncident = false;
    var header = '<tr>';

    for (i = 0; i < self.columnNames.length; i++) {
      columnName = self.columnNames[i];
      definition = columnDefinitions[columnName];
      if (!definition) {
        console.error("Unknown column: " + columnName);
        continue;
      }

      isIncident = (columnName.indexOf("incident") === 0);

      header += '<th';
      header += ' id="column_' + columnName + '"';
      if (isIncident)
        header += ' class="incident" style="display:none;"';
      header += '>';
      header += definition.header;
      header += '</th>';
    }

    header += '</tr>';

    return header;
  }

  function drawTableBody() {
    var html = "";
    var x, y, serverId, server, event, columnName, definition;
    var haveIncident = self.rawData["haveIncident"];

    for (x = 0; x < self.rawData["events"].length; ++x) {
      event = self.rawData["events"][x];
      serverId = event["serverId"];
      server = self.rawData["servers"][serverId];

      html += "<tr>";
      for (y = 0; y < self.columnNames.length; y++) {
        columnName = self.columnNames[y];
        definition = columnDefinitions[columnName];
        if (!definition)
          continue;
        html += definition.body(event, server);
      }
      html += "</tr>";
    }

    return html;
  }

  function switchSort() {
    var icon;
    var currentOrder = self.baseQuery.sortOrder;

    if (currentOrder == hatohol.DATA_QUERY_OPTION_SORT_DESCENDING) {
      icon = "up";
      self.baseQuery.sortOrder = hatohol.DATA_QUERY_OPTION_SORT_ASCENDING;
      self.userConfig.saveValue('events.sort.order', self.baseQuery.sortOrder);
      self.startConnection(getQuery({ page: self.currentPage }), updateCore);
    } else {
      icon = "down";
      self.baseQuery.sortOrder = hatohol.DATA_QUERY_OPTION_SORT_DESCENDING;
      self.userConfig.saveValue('events.sort.order', self.baseQuery.sortOrder);
      self.startConnection(getQuery({ page: self.currentPage }), updateCore);
    }
  }

  function drawTableContents() {
    $("#table thead").empty();
    $("#table thead").append(drawTableHeader());
    $("#table tbody").empty();
    $("#table tbody").append(drawTableBody());

    if (self.rawData["haveIncident"]) {
      $(".incident").show();
    } else {
      $(".incident").hide();
    }

    $('.incident.selectable').on('click', function() {
      $(this).toggleClass('selected');
      setupChangeIncidentMenu();
    });

    setupSortColumn();

    function setupSortColumn() {
      var th = $("#column_time");
      var icon = "down";
      if (self.baseQuery.sortOrder == hatohol.DATA_QUERY_OPTION_SORT_ASCENDING)
        icon = "up";
      th.find("i.sort").remove();
      th.append(" <i class='sort glyphicon glyphicon-collapse-" + icon +"'></i>");
      th.click(function() {
        switchSort();
      });
    }
  }

  function setupChangeIncidentMenu() {
    var selected = $('.incident.selectable.selected');
    if (selected.length > 0) {
      $("#change-incident").removeAttr("disabled", "disabled").parents('form').show();
    } else {
      $("#change-incident").attr("disabled", "disabled").parents('form').hide();
    }
  }

  function setupStatictics() {
    // Assign/UnAssign events statistics
    var numOfImportantEvents = self.rawSummaryData["numOfImportantEvents"];
    var numOfAssignedEvents = self.rawSummaryData["numOfAssignedImportantEvents"];
    var numOfUnAssignedEvents = numOfImportantEvents - numOfAssignedEvents;
    $("#numOfUnAssignedEvents").text(numOfUnAssignedEvents);
    var unAssignedEventsPercentage = 0;
    if (numOfImportantEvents > 0)
      unAssignedEventsPercentage =
        (numOfUnAssignedEvents / numOfImportantEvents * 100).toFixed(2);
    $("#unAssignedEventsPercentage").text(unAssignedEventsPercentage + "%");
    $("#unAssignedEventsPercentage").css("width", unAssignedEventsPercentage+"%");

    // Important events statistics
    $("#numOfImportantEvents").text(numOfImportantEvents);
    var numOfImportantEventOccurredHosts =
          self.rawSummaryData["numOfImportantEventOccurredHosts"];
    $("#numOfImportantEventOccurredHosts").text(numOfImportantEventOccurredHosts);
    var numOfAllHosts =
          self.rawSummaryData["numOfAllHosts"];
    var importantEventOccurredHostsPercentage = 0;
    if (numOfAllHosts > 0)
      importantEventOccurredHostsPercentage =
        (numOfImportantEventOccurredHosts / numOfAllHosts * 100)
        .toFixed(2);
    $("#importantEventOccurredHostsPercentage").text(importantEventOccurredHostsPercentage+"%");
    $("#importantEventOccurredHostsPercentage")
      .css("width", importantEventOccurredHostsPercentage+"%");
  }

  function setupTableColor() {
    var severityRanks = self.rawSeverityRankData["SeverityRanks"];
    var severity, color;
    if (!severityRanks)
      return;
    for (var x = 0; x < severityRanks.length; ++x) {
      severity = severityRanks[x].status;
      color = severityRanks[x].color;
      $('td.severity' + severity).css("background-color", color);
    }
  }

  function updateSummary(reply) {
    if (!$("#SummarySidebar").is(":visible"))
      return;

    if (reply)
      self.rawSummaryData = reply;

    setupStatictics();
  }

  function updateCore(reply) {
    self.rawData = reply;
    self.userConfig.setServers(reply.servers);
    self.durations = parseData(self.rawData);

    setupFilterValues();
    setupHandlingMenu();
    drawTableContents();
    setupTableColor();
    updatePager();
    updateFilteringResult();
    setLoading(false);
    if (self.currentPage === 0)
      self.enableAutoRefresh(load, self.reloadIntervalSeconds);
  }
};

EventsView.prototype = Object.create(HatoholMonitoringView.prototype);
EventsView.prototype.constructor = EventsView;
