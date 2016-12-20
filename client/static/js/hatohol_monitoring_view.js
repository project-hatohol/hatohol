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

window.onerror = function(errorMsg, fileName, lineNumber) {
  var place = "[" + fileName + ":" + lineNumber + "]";
  console.error(place + " " + errorMsg);
  HatoholMonitoringView.prototype.setStatus({
    "class": "Danger",
    "label": gettext("Error"),
    "lines": [place, errorMsg],
  });
};

var HatoholMonitoringView = function(userProfile) {
  var self = this;
  self.connector = null;
  self.reloadTimerId = null;
  if (!userProfile)
    throw new Error("No userProfile");
  self.userProfile = userProfile;
};

HatoholMonitoringView.prototype.getTargetServerId = function() {
  var id = $("#select-server").val();
  if (id === "")
    id = null;
  return id;
};

HatoholMonitoringView.prototype.getTargetHostgroupId = function() {
  var id = $("#select-host-group").val();
  if (id === "")
    id = null;
  return id;
};

HatoholMonitoringView.prototype.getTargetHostId = function() {
  var id = $("#select-host").val();
  if (id === "")
    id = null;
  return id;
};

HatoholMonitoringView.prototype.getTargetHostname = function() {
  return $("#select-hostname").val();
};

HatoholMonitoringView.prototype.getTargetHostgroupName = function() {
  return $("#select-host-group-name").val();
};

HatoholMonitoringView.prototype.getTargetTriggerBrief = function() {
  return $("#select-trigger-brief").val();
};

HatoholMonitoringView.prototype.getTargetAppName = function() {
  return $("#select-application").val();
};

HatoholMonitoringView.prototype.setFilterCandidates =
  function(target, candidates)
{
  var x;
  var html = "<option value=''>---------</option>";

  target.empty().append(html);

  if (candidates) {
    target.removeAttr("disabled");
    for (x = 0; candidates && x < candidates.length; ++x) {
      var option = $("<option/>");
      if (typeof candidates[x] == "string") {
        option.text(candidates[x]);
      } else if (typeof candidates[x] == "object") {
        option.text(candidates[x].label);
        option.attr("value", candidates[x].value);
      }
      target.append(option);
    }
  } else {
    target.attr("disabled", "disabled");
  }
};

HatoholMonitoringView.prototype.compareFilterLabel = function(a, b) {
  if (a.label < b.label)
    return -1;
  if (a.label > b.label)
    return 1;
  if (a.id < b.id)
    return -1;
  if (a.id > b.id)
    return 1;
  return 0;
};

HatoholMonitoringView.prototype.setServerFilterCandidates = function(servers)
{
  var id, serverLabels = [], current;
  var serverSelector = $('#select-server');

  current = serverSelector.val();
  for (id in servers) {
    serverLabels.push({
      label: getNickName(servers[id], id),
      value: id
    });
  }
  serverLabels.sort(this.compareFilterLabel);
  this.setFilterCandidates(serverSelector, serverLabels);
  serverSelector.val(current);
};

HatoholMonitoringView.prototype.setHostgroupFilterCandidates =
  function(servers, serverId)
{
  var id, server, groups, groupLabels = [], current;
  var groupSelector =$( '#select-host-group');

  current = groupSelector.val();
  if (!serverId)
    serverId = this.getTargetServerId();

  this.setFilterCandidates(groupSelector);

  if (!servers || !servers[serverId])
    return;

  server = servers[serverId];
  groups = server.groups;
  for (id in groups) {
    groupLabels.push({
      label: groups[id].name,
      value: id
    });
  }
  groupLabels.sort(this.compareFilterLabel);
  this.setFilterCandidates(groupSelector, groupLabels);
  groupSelector.val(current);
};

HatoholMonitoringView.prototype.setHostFilterCandidates =
  function(servers, serverId, withoutSelfMonitor)
{
  var id, server, hosts, hostLabels = [], current;
  var hostSelector = $('#select-host');

  current = hostSelector.val();
  if (!serverId)
    serverId = this.getTargetServerId();

  this.setFilterCandidates(hostSelector);

  if (!servers || !servers[serverId])
    return;

  server = servers[serverId];
  hosts = server.hosts;
  for (id in hosts) {
    if (withoutSelfMonitor && (withoutSelfMonitor === true) && (id == "__SELF_MONITOR"))
      continue;
    hostLabels.push({
      label: getHostName(server, id),
      value: id
    });
  }
  hostLabels.sort(this.compareFilterLabel);
  this.setFilterCandidates(hostSelector, hostLabels);
  hostSelector.val(current);
};

HatoholMonitoringView.prototype.setHostnameFilterCandidates =
  function(servers, withoutSelfMonitor)
{
  var id, server, hosts, hostLabels = [], current;
  var hostnameSelector = $('#select-hostname');
  var tempStorage = {};

  current = hostnameSelector.val();

  this.setFilterCandidates(hostnameSelector);

  if (!servers)
    return;

  for (var serverId in servers) {
    server = servers[serverId];
    for (id in server.hosts) {
      if (withoutSelfMonitor && (withoutSelfMonitor === true) && (id == "__SELF_MONITOR"))
        continue;
      var hostChioce = {
        label: getHostName(server, id),
        value: getHostName(server, id)
      };
      tempStorage[getHostName(server, id)] = hostChioce;
    }
  }
  for (var key in tempStorage) {
    hostLabels.push(tempStorage[key]);
  }
  hostLabels.sort(this.compareFilterLabel);
  this.setFilterCandidates(hostnameSelector, hostLabels);
  hostnameSelector.val(current);
};

HatoholMonitoringView.prototype.setHostgroupNameFilterCandidates =
  function(servers)
{
  var id, server, groups, groupLabels = [], current;
  var groupNameSelector =$('#select-host-group-name');
  var tempStorage = {};

  current = groupNameSelector.val();

  this.setFilterCandidates(groupNameSelector);

  if (!servers)
    return;

  for (var serverId in servers) {
    server = servers[serverId];
    groups = server.groups;
    for (id in groups) {
      var groupChioce = {
        label: groups[id].name,
        value: groups[id].name
      };
      tempStorage[groups[id].name] = groupChioce;
    }
  }
  for (var key in tempStorage) {
    groupLabels.push(tempStorage[key]);
  }
  groupLabels.sort(this.compareFilterLabel);
  this.setFilterCandidates(groupNameSelector, groupLabels);
  groupNameSelector.val(current);
};

HatoholMonitoringView.prototype.setTriggerBriefsFilterCandidates =
  function(triggerBriefs)
{
  var id, server, brief, briefLabels = [], current;
  var triggerBriefSelector = $('#select-trigger-brief');

  current = triggerBriefSelector.val();

  this.setFilterCandidates(triggerBriefSelector);

  if (!triggerBriefs)
    return;

  var briefs = triggerBriefs.briefs;
  var length = briefs.length;
  for (var i = 0; i < length; i++) {
    brief = briefs[i].brief;
    var briefChioce = {
      label: brief,
      value: brief
    };
    briefLabels.push(briefChioce);
  }
  briefLabels.sort(this.compareFilterLabel);
  this.setFilterCandidates(triggerBriefSelector, briefLabels);
  triggerBriefSelector.val(current);
};

HatoholMonitoringView.prototype.setApplicationFilterCandidates =
function(candidates)
{
  var applicationSelector = $("#select-application");
  var current = applicationSelector.val();
  this.setFilterCandidates(applicationSelector, candidates);
  applicationSelector.val(current);
};

HatoholMonitoringView.prototype.getHostFilterQuery = function(isOverview) {
  var query = {};
  var serverId = this.getTargetServerId();
  var hostgroupId = this.getTargetHostgroupId();
  var hostId = this.getTargetHostId();
  var hostname = this.getTargetHostname();
  var hostgroupName = this.getTargetHostgroupName();
  var triggerBrief = this.getTargetTriggerBrief();
  if (serverId)
    query.serverId = serverId;
  else
    query.serverId = isOverview ? INVALID_SERVER_ID : ALL_SERVERS;
  query.hostgroupId = hostgroupId ? hostgroupId : "*";
  query.hostId = hostId ? hostId : "*";
  query.hostname = hostname;
  query.hostgroupName = hostgroupName;
  query.triggerBrief = triggerBrief;
  return query;
};

HatoholMonitoringView.prototype.setupHostQuerySelectorCallback =
  function(loadFunc, serverSelectorId, hostgroupSelectorId, hostSelectorId, applicationId)
{
  // server
  if (serverSelectorId) {
    $(serverSelectorId).change(function() {
      resetHostQuerySelector(hostgroupSelectorId);
      resetHostQuerySelector(hostSelectorId);
      resetHostQuerySelector(applicationId);
      loadFunc();
    });
  }

  // hostgroup
  if (hostgroupSelectorId) {
    $(hostgroupSelectorId).change(function() {
      resetHostQuerySelector(hostSelectorId);
      loadFunc();
    });
  }

  // host
  if (hostSelectorId) {
    $(hostSelectorId).change(function() {
      loadFunc();
    });
  }

  // application
  if (applicationId) {
    $(applicationId).change(function() {
      loadFunc();
    });
  }

  function resetHostQuerySelector(selectorId) {
    if (!selectorId)
      return;
    $(selectorId).val("---------");
  }
};

HatoholMonitoringView.prototype.setStatus = function (value) {
  var elem;
  var x;
  var s;

  if ("class" in value) {
    $("#sts button").attr("class", "navbar-btn btn btn-" + value["class"]);
  }

  if ("label" in value) {
    elem = $("#sts button span:first");
    elem.empty();
    elem.append(value["label"]);
  }

  if (value.lines && value.lines.length > 0) {
    s = "";
    for (x = 0; x < value["lines"].length; ++x) {
      s += "<li>" + value["lines"][x] + "</li>";
    }

    elem = $("#sts ul");
    elem.empty();
    elem.append(s);
    $("#sts button").attr("data-toggle", "dropdown");
  } else {
    $("#sts button").removeAttr("data-toggle");
  }
};

HatoholMonitoringView.prototype.updateScreen =
  function (reply, completionCallback, callbackParam)
{
  this.setStatus({
    "class" : "warning",
    "label" : gettext("DRAW"),
    "lines" : [ gettext("Drawing") ],
  });

  completionCallback(reply, callbackParam);

  this.setStatus({
    "class" : "success",
    "label" : pgettext("Indicator", "DONE"),
    "lines" : [],
  });
};

HatoholMonitoringView.prototype.startConnection =
  function (query, completionCallback, callbackParam, connParam)
{
  var self = this;

  self.setStatus({
    "class" : "warning",
    "label" : gettext("LOAD"),
    "lines" : [ gettext("Communicating with backend") ],
  });

  connParam = $.extend({
    url: '/' + query,
    replyCallback: function(reply, parser) {
      self.updateScreen(reply, completionCallback, callbackParam);
    },
    parseErrorCallback: function(reply, parser) {
      // We assume the parser is HatoholReplyParser.
      var msg = parser.getMessage();
      if (!msg)
        msg = gettext("Failed to parse the received packet.");
      hatoholErrorMsgBox(msg);

      self.setStatus({
        "class" : "danger",
        "label" : gettext("ERROR"),
        "lines" : [ msg ],
      });
    }
  }, connParam || {});

  if (self.connector)
    self.connector.start(connParam);
  else
    self.connector = new HatoholConnector(connParam);
};

HatoholMonitoringView.prototype.setAutoReload =
  function(reloadFunc, intervalSeconds)
{
  this.clearAutoReload();
  if (intervalSeconds)
    this.reloadTimerId = setTimeout(reloadFunc, intervalSeconds * 1000);
};

HatoholMonitoringView.prototype.clearAutoReload = function()
{
  if (this.reloadTimerId)
    clearTimeout(this.reloadTimerId);
  this.reloadTimerId = null;
};

HatoholMonitoringView.prototype.setupCheckboxForDelete =
  function(jQObjDeleteButton, hasPrivilege)
{
  var self = this;
  var numSelected = 0;
  $(".selectcheckbox").attr("checked", false);
  jQObjDeleteButton.attr("disabled", true);
  $(".selectcheckbox").change(function() {
    check = $(this).is(":checked");
    var prevNumSelected = numSelected;
    if (check)
      numSelected += 1;
    else
      numSelected -= 1;
    if (prevNumSelected === 0 && numSelected == 1)
      jQObjDeleteButton.attr("disabled", false);
    else if (prevNumSelected == 1 && numSelected === 0)
      jQObjDeleteButton.attr("disabled", true);
  });
};

HatoholMonitoringView.prototype.setupHostFilters = function(servers, query, withoutSelfMonitor) {
  this.setServerFilterCandidates(servers);
  if (query && ("serverId" in query)) {
    if (query.serverId == INVALID_SERVER_ID) {
      $("#select-server").val("");
    } else {
      $("#select-server").val(query.serverId);
    }
  }

  this.setHostgroupFilterCandidates(servers);
  if (query && ("hostgroupId" in query)) {
    if (query.hostgroupId == "*") {
      $("#select-host-group").val("");
    } else {
      $("#select-host-group").val(query.hostgroupId);
    }
  }

  this.setHostFilterCandidates(servers, this.getTargetServerId(), withoutSelfMonitor);

  if (query && ("hostId" in query)) {
    if (query.hostId == "*") {
      $("#select-host").val("");
    } else {
      $("#select-host").val(query.hostId);
    }
  }

  this.setHostnameFilterCandidates(servers, withoutSelfMonitor);

  if (query && ("hostname" in query)) {
    if (query.hostname === "") {
      $("#select-hostname").val("");
    } else {
      $("#select-hostname").val(query.hostname);
    }
  }

  this.setHostgroupNameFilterCandidates(servers);

  if (query && ("hostgroupName" in query)) {
    if (query.hostgroupName === "") {
      $("#select-host-group-name").val("");
    } else {
      $("#select-host-group-name").val(query.hostgroupName);
    }
  }
};

HatoholMonitoringView.prototype.displayUpdateTime =
function()
{
  var date = function() {
    var time = new Date();
      return {
        getCurrentTime: function() {
          return time.toLocaleString();
        }
      };
  }();
  $("#update-time").empty();
  $("#update-time").append(gettext("Last Update:") + " " + date.getCurrentTime());
};

HatoholMonitoringView.prototype.enableAutoRefresh =
function(reloadFunc, reloadIntervalSeconds)
{
  var button = $("#toggleAutoRefreshButton");
  button.removeClass("btn-default");
  button.addClass("btn-success");
  button.addClass("active");
  button.children(".toggleAutoRefreshStatus").html(gettext("Monitoring"));
  this.setAutoReload(reloadFunc, reloadIntervalSeconds);
};

HatoholMonitoringView.prototype.disableAutoRefresh =
function()
{
  var button = $("#toggleAutoRefreshButton");
  this.clearAutoReload();
  button.removeClass("btn-success");
  button.removeClass("active");
  button.addClass("btn-default");
  button.children(".toggleAutoRefreshStatus").html(gettext("Pause"));
};

HatoholMonitoringView.prototype.setupToggleAutoRefreshButtonHandler =
function(reloadFunc, intervalSeconds)
{
  var self = this;
  $("#toggleAutoRefreshButton").on("click", function() {
    if ($(this).hasClass("active")) {
      self.disableAutoRefresh();
    } else {
      self.enableAutoRefresh(reloadFunc, intervalSeconds);
    }
  });
};

HatoholMonitoringView.prototype.showToggleAutoRefreshButton =
function()
{
  $("#toggleAutoRefreshButton").show();
};
