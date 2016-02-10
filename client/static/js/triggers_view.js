/*
 * Copyright (C) 2013-2014 Project Hatohol
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

var TriggersView = function(userProfile, options) {
  var self = this;
  var rawData;

  self.reloadIntervalSeconds = 60;
  self.currentPage = 0;
  self.baseQuery = {
    limit: 50,
  };
  $.extend(self.baseQuery, getTriggersQueryInURI());
  self.lastQuery = undefined;
  self.lastQuickFilter = {};
  self.showToggleAutoRefreshButton();
  self.setupToggleAutoRefreshButtonHandler(load, self.reloadIntervalSeconds);
  self.rawSeverityRankData = {};
  self.severityRanksMap = {};
  self.options = options || {};

  setupToggleFilter();
  if (self.options.disableTimeRangeFilter) {
   // Don't enable datetimepicker for tests.
  } else {
    setupTimeRangeFilter();
  }

  var triggerPropertyChoices = {
    severity: [
      { value: "0", label: gettext("Not classified") },
      { value: "1", label: gettext("Information") },
      { value: "2", label: gettext("Warning") },
      { value: "3", label: gettext("Average") },
      { value: "4", label: gettext("High") },
      { value: "5", label: gettext("Disaster") },
    ],
  };

  // call the constructor of the super class
  HatoholMonitoringView.apply(this, [userProfile]);

  self.pager = new HatoholPager();
  self.userConfig = new HatoholUserConfig();
  start();

  function start() {
    $.when(loadUserConfig(), loadSeverityRank()).done(function() {
      load();
    }).fail(function() {
      hatoholInfoMsgBox(gettext("Failed to get the configuration!"));
      load(); // Ensure to work with the default config
    });
  }

  function loadUserConfig() {
    var deferred = new $.Deferred;
    self.userConfig.get({
      itemNames:['num-triggers-per-page'],
      successCallback: function(conf) {
        self.baseQuery.limit =
          self.userConfig.findOrDefault(conf, 'num-triggers-per-page',
                                        self.baseQuery.limit);
        updatePager();
        setupFilterValues();
        setupCallbacks();
        deferred.resolve();
      },
      connectErrorCallback: function(XMLHttpRequest) {
        showXHRError(XMLHttpRequest);
        deferred.reject();
      },
    });
    return deferred.promise();
  }

  function loadSeverityRank() {
    var deferred = new $.Deferred;
    new HatoholConnector({
      url: "/severity-rank",
      request: "GET",
      replyCallback: function(reply, parser) {
        var i, severityRanks, rank;
        var choices = triggerPropertyChoices.severity;
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

  function showXHRError(XMLHttpRequest) {
    var errorMsg = "Error: " + XMLHttpRequest.status + ": " +
      XMLHttpRequest.statusText;
    hatoholErrorMsgBox(errorMsg);
  }

  function saveConfig(items) {
    self.userConfig.store({
      items: items,
      successCallback: function() {
        // we just ignore it
      },
      connectErrorCallback: function(XMLHttpRequest) {
        showXHRError(XMLHttpRequest);
      },
    });
  }

  function updatePager() {
    self.pager.update({
      numTotalRecords: rawData ? rawData["totalNumberOfTriggers"] : -1,
      numRecordsPerPage: self.baseQuery.limit,
      selectPageCallback: function(page) {
        if (self.pager.numRecordsPerPage != self.baseQuery.limit) {
          self.baseQuery.limit = self.pager.numRecordsPerPage;
          saveConfig({'num-triggers-per-page': self.baseQuery.limit});
        }
        load({page: page});
      }
    });
  }

  function resetTriggerPropertyFilter(type) {
    var candidates = triggerPropertyChoices[type];
    var option;
    var currentId = $("#select-" + type).val();

    $("#select-" + type).empty();

    $("#select-" + type).empty();

    option = $("<option/>", {
      text: "---------",
      value: "",
    }).appendTo("#select-" + type);

    $.map(candidates, function(candidate) {
      var option;

      option = $("<option/>", {
        text: candidate.label,
        value: candidate.value
      }).appendTo("#select-" + type);
    });

    $("#select-" + type).val(currentId);
  }

  var status_choices = [
    gettext("OK"),
    gettext("Problem"),
    gettext("Unknown")
  ];

  function setupFilterValues(servers, query) {
    if (!servers && rawData && rawData.servers)
      servers = rawData.servers;

    if (!query)
      query = self.lastQuery ? self.lastQuery : self.baseQuery;

    self.setupHostFilters(servers, query);
    resetTriggerPropertyFilter("severity");

    if ("minimumSeverity" in query)
      $("#select-severity").val(query.minimumSeverity);
    if ("status" in query)
      $("#select-status").val(query.status);
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

  function setupToggleFilter() {
    $("#hideDiv").hide();
    $('#hide').click(function(){
      $("#hideDiv").slideToggle();
      $("#filter-right-glyph").toggle();
      $("#filter-down-glyph").toggle();
    });
  }

  function setupCallbacks() {
    $('#select-server').change(function() {
      resetHostQuerySelector('#select-host-group');
      resetHostQuerySelector('#select-host');
      setupFilterValues();

      function resetHostQuerySelector(selectorId) {
        if (!selectorId)
          return;
        $(selectorId).val("---------");
      }
    });
    $('button.reset-apply-all-filter').click(function() {
      resetTimeRangeFilter();
      resetQuickFilter();
      load({applyFilter: true});
    });

    $('button.btn-apply-all-filter').click(function() {
      load({applyFilter: true});
    });
  }

  function setLoading(loading) {
    if (loading) {
      $("#begin-time").attr("disabled", "disabled");
      $("#end-time").attr("disabled", "disabled");
      $("#select-severity").attr("disabled", "disabled");
      $("#select-status").attr("disabled", "disabled");
      $("#select-server").attr("disabled", "disabled");
      $("#select-hostgroup").attr("disabled", "disabled");
      $("#select-host").attr("disabled", "disabled");
    } else {
      $("#begin-time").removeAttr("disabled");
      $("#end-time").removeAttr("disabled");
      $("#select-severity").removeAttr("disabled");
      $("#select-status").removeAttr("disabled");
      $("#select-server").removeAttr("disabled");
      if ($("#select-hostgroup option").length > 1)
        $("#select-hostgroup").removeAttr("disabled");
      if ($("#select-host option").length > 1)
        $("#select-host").removeAttr("disabled");
    }
  }

  function getTriggerName(trigger) {
    var extendedInfo, name;

    try {
      extendedInfo = JSON.parse(trigger["extendedInfo"]);
      name = extendedInfo["expandedDescription"];
    } catch(e) {
    }
    return name ? name : trigger["brief"];
  }

  function getSeverityClass(trigger) {
    var status     = trigger["status"];
    var severity   = trigger["severity"];
    var severityClass = "severity";

    if (status == hatohol.TRIGGER_STATUS_PROBLEM)
      return severityClass += escapeHTML(severity);
    else
      return "";
  }

  function drawTableBody(replyData) {
    var serverName, hostName, clock, status, severity, triggerName;
    var html, server, trigger, severityClass;
    var x, serverId, hostId;

    html = "";
    for (x = 0; x < replyData["triggers"].length; ++x) {
      trigger    = replyData["triggers"][x];
      serverId   = trigger["serverId"];
      hostId     = trigger["hostId"];
      server     = replyData["servers"][serverId];
      nickName   = getNickName(server, serverId);
      hostName   = getHostName(server, hostId);
      clock      = trigger["lastChangeTime"];
      status     = trigger["status"];
      severity   = trigger["severity"];
      severityClass = getSeverityClass(trigger);
      triggerName = getTriggerName(trigger);

      html += "<tr><td class='" + severityClass +"'>"
        + escapeHTML(nickName) + "</td>";
      html += "<td class='" + severityClass +
        "' data-sort-value='" + escapeHTML(severity) + "'>" +
        triggerPropertyChoices.severity[Number(severity)].label + "</td>";
      html += "<td class='status" + escapeHTML(status);
      if (severityClass) {
        html += " " + severityClass;
      }
      html += "' data-sort-value='" + escapeHTML(status) + "'>" +
        status_choices[Number(status)] + "</td>";
      html += "<td class='" + severityClass +
        "' data-sort-value='" + escapeHTML(clock) + "'>" +
        formatDate(clock) + "</td>";
      html += "<td class='" + severityClass + "'>" +
        escapeHTML(hostName) + "</td>";
      html += "<td class='" + severityClass + "'>"
	+ "<a href='ajax_events?serverId=" + escapeHTML(serverId)
	+ "&triggerId=" + escapeHTML(trigger["id"]) + "'>"
	+ escapeHTML(triggerName)
	+ "</a></td>";
      html += "</tr>";
    }

    return html;
  }

  function drawTableContents(data) {
    $("#table tbody").empty();
    $("#table tbody").append(drawTableBody(data));
  }

  function updateCore(reply) {
    rawData = reply;

    drawTableContents(rawData);
    updatePager();
    setupFilterValues();
    setupTableColor();
    setLoading(false);
    self.setAutoReload(load, self.reloadIntervalSeconds);
  }

  function getTriggersQueryInURI() {
    var knownKeys = [
      "serverId", "hostgroupId", "hostId",
      "limit", "offset",
      "minimumSeverity", "status",
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
    if ($("#select-severity").val())
      query.minimumSeverity = $("#select-severity").val();

    if ($("#select-status").val())
      query.status = $("#select-status").val();

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
    options = options || {};
    if (isNaN(options.page))
      options.page = 0;
    var query = $.extend({}, self.baseQuery, {
      offset:          self.baseQuery.limit * options.page
    });
    if (options.applyFilter) {
      self.lastQuickFilter = getQuickFilter();
      $.extend(query, self.lastQuickFilter);
    }
    self.lastQuery = query;
    return 'trigger?' + $.param(query);
  };

  function resetTimeRangeFilter() {
    $("#begin-time").next(".clear-button").trigger('click');
    $("#end-time").next(".clear-button").trigger('click');
  }

  function resetQuickFilter() {
    $("#select-severity").val("");
    $("#select-status").val("");
    $("#select-server").val("");
    $("#select-host-group").val("");
    $("#select-host").val("");
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

  function load(options) {
    options = options || {};
    self.displayUpdateTime();
    setLoading(true);
    if (!isNaN(options.page)) {
      self.currentPage = options.page;
    }
    self.startConnection(getQuery(options), updateCore);
    self.pager.update({ currentPage: self.currentPage });
    $(document.body).scrollTop(0);
  }
};

TriggersView.prototype = Object.create(HatoholMonitoringView.prototype);
TriggersView.prototype.constructor = TriggersView;
