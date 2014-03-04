/*
 * Copyright (C) 2013-2014 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Hatohol. If not, see <http://www.gnu.org/licenses/>.
 */

window.onerror = function(errorMsg, fileName, lineNumber) {
  var place = "[" + fileName + ":" + lineNumber + "]";
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
};

HatoholMonitoringView.prototype.getTargetServerId = function(selectorId) {
  var id;
  if (!selectorId)
    selectorId = "#select-server";
  id = $(selectorId).val();
  if (id == "---------")
    id = null;
  return id;
};

HatoholMonitoringView.prototype.getTargetHostGroupId = function(selectorId) {
  var id;
  if (!selectorId)
    selectorId = "#select-hostgroup";
  id = $("#select-hostgroup").val();
  if (id == "---------")
    id = null;
  return id;
};

HatoholMonitoringView.prototype.getTargetHostId = function(selectorId) {
  var id;
  if (!selectorId)
    selectorId = "#select-host";
  id = $("#select-host").val();
  if (id == "---------")
    id = null;
  return id;
};

HatoholMonitoringView.prototype.setFilterCandidates =
  function(target, candidates)
{
  var x;
  var html = "<option>---------</option>";

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

HatoholMonitoringView.prototype.setServerFilterCandidates =
  function(servers, selectorId)
{
  var id, serverLabels = [], current;
  if (!selectorId)
    selectorId = '#select-server';
  current = $(selectorId).val();
  for (id in servers) {
    serverLabels.push({
      label: getServerName(servers[id], id),
      value: id
    });
  }
  serverLabels.sort(this.compareFilterLabel);
  this.setFilterCandidates($(selectorId), serverLabels);
  $(selectorId).val(current);
};

HatoholMonitoringView.prototype.setHostGroupFilterCandidates =
  function(servers, serverId, selectorId)
{
  var id, server, groups, groupLabels = [], current;

  if (!selectorId)
    selectorId = '#select-host-group';
  current = $(selectorId).val();
  if (!serverId)
    serverId = this.getTargetServerId();

  this.setFilterCandidates($(selectorId));

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
  this.setFilterCandidates($(selectorId), groupLabels);
  $(selectorId).val(current);
};

HatoholMonitoringView.prototype.setHostFilterCandidates =
  function(servers, serverId, selectorId)
{
  var id, server, hosts, hostLabels = [], current;

  if (!selectorId)
    selectorId = '#select-host';
  current = $(selectorId).val();
  if (!serverId)
    serverId = this.getTargetServerId();

  this.setFilterCandidates($(selectorId));

  if (!servers || !servers[serverId])
    return;

  server = servers[serverId];
  hosts = server.hosts;
  for (id in hosts) {
    hostLabels.push({
      label: getHostName(server, id),
      value: id
    });
  }
  hostLabels.sort(this.compareFilterLabel);
  this.setFilterCandidates($(selectorId), hostLabels);
  $(selectorId).val(current);
};

HatoholMonitoringView.prototype.addHostQuery = function(query) {
    var serverId = this.getTargetServerId();
    var hostGroupId = this.getTargetHostGroupId();
    var hostId = this.getTargetHostId();
    if (serverId)
      query.serverId = serverId;
    if (hostGroupId)
      query.hostGroupId = hostGroupId;
    if (hostId)
      query.hostId = hostId;
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
    "label" : gettext("DONE"),
    "lines" : [],
  });
};

HatoholMonitoringView.prototype.startConnection =
  function (query, completionCallback, callbackParam)
{
  var self = this;
  
  self.setStatus({
    "class" : "warning",
    "label" : gettext("LOAD"),
    "lines" : [ gettext("Communicating with backend") ],
  });

  var connParam =  {
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
  };

  if (self.connector)
    self.connector.start(connParam);
  else
    self.connector = new HatoholConnector(connParam);
};

HatoholMonitoringView.prototype.setAutoReload =
  function(reloadFunc, intervalSeconds)
{
  if (this.reloadTimerId)
    clearTimeout(this.reloadTimerId);
  if (intervalSeconds)
    this.reloadTimerId = setTimeout(reloadFunc, intervalSeconds * 1000);
};
