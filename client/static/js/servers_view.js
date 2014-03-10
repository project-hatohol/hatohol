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

var ServersView = function(userProfile) {
  var self = this;
  var numSelected = 0;
  var serverIds = new Array();

  // call the constructor of the super class
  HatoholMonitoringView.apply(userProfile);

  if (userProfile.hasFlag(hatohol.OPPRVLG_CREATE_SERVER))
    $("#add-server-button").show();
  if (userProfile.hasFlag(hatohol.OPPRVLG_DELETE_SERVER))
    $("#delete-server-button").show();
  self.startConnection('server', updateCore);

  $("#table").stupidtable();
  $("#table").bind('aftertablesort', function(event, data) {
    var th = $(this).find("th");
    th.find("i.sort").remove();
    var icon = data.direction === "asc" ? "up" : "down";
    th.eq(data.column).append("<i class='sort glyphicon glyphicon-arrow-" + icon +"'></i>");
  });

  $("#add-server-button").click(function() {
    new HatoholServerEditDialog({
      operator: userProfile.user,
      succeededCallback: addOrUpdateSucceededCb
    });
  });

  $("#delete-server-button").click(function() {
    var msg = gettext("Do you delete the selected items ?");
    hatoholNoYesMsgBox(msg, deleteServers);
  });

  function addOrUpdateSucceededCb() {
    self.startConnection('server', updateCore);
  };

  function setupCheckboxHandler() {
    $(".selectcheckbox").change(function() {
      check = $(this).is(":checked");
      var prevNumSelected = numSelected;
      if (check)
        numSelected += 1;
      else
        numSelected -= 1;
      if (prevNumSelected == 0 && numSelected == 1)
        $("#delete-server-button").attr("disabled", false);
      else if (prevNumSelected == 1 && numSelected == 0)
        $("#delete-server-button").attr("disabled", true);
    });

    if (userProfile.hasFlag(hatohol.OPPRVLG_DELETE_SERVER)) {
      $(".delete-selector").show();
    }
  }

  function setupEditButtons(reply)
  {
    var i, id, servers = reply["servers"], serversMap = {};

    for (i = 0; i < servers.length; ++i)
      serversMap[servers[i]["id"]] = servers[i];

    for (i = 0; i < servers.length; ++i) {
      id = "#edit-server" + servers[i]["id"];
      $(id).click(function() {
        var serverId = this.getAttribute("serverId");
        new HatoholServerEditDialog({
          operator: userProfile.user,
          succeededCallback: addOrUpdateSucceededCb,
          targetServer: serversMap[serverId]
        });
      });
    }

    if (userProfile.hasFlag(hatohol.OPPRVLG_UPDATE_SERVER) ||
        userProfile.hasFlag(hatohol.OPPRVLG_UPDATE_ALL_SERVER))
    {
      $(".edit-server-column").show();
    }
  }

  function parseResult(data) {
    var msg;
    var malformed =false;
    if (data.result == undefined)
      malformed = true;
    if (!malformed && !data.result && data.message == undefined)
      malformed = true;
    if (malformed) {
      msg = "The returned content is malformed: " +
        "Not found 'result' or 'message'.\n" +
        JSON.stringify(data);
      hatoholErrorMsgBox(msg);
      return false;
    }
    if (!data.result) {
      msg = "Failed:\n" + data.message;
      hatoholErrorMsgBox(msg);
      return false;
    }
    if (data.id == undefined) {
      msg = "The returned content is malformed: " +
        "'result' is true, however, 'id' missing.\n" +
        JSON.stringfy(data);
      hatoholErrorMsgBox(msg);
      return false;
    }
    return true;
  }

  function deleteServers() {
    var delId = [], serverId;
    var checkbox = $(".selectcheckbox");
    $(this).dialog("close");
    for (var i = 0; i < checkbox.length; i++) {
      if (!checkbox[i].checked)
        continue;
      serverId = checkbox[i].getAttribute("serverId");
      delId.push(serverId);
    }
    new HatoholItemRemover({
      id: delId,
      type: "server",
      completionCallback: function() {
        self.startConnection("server", updateCore);
      },
    });
    hatoholInfoMsgBox("Deleting...");
  }

  function getServerTypeLabel(type) {
    switch(type) {
    case hatohol.MONITORING_SYSTEM_ZABBIX:
      return gettext("Zabbix");
    case hatohol.MONITORING_SYSTEM_NAGIOS:
      return gettext("Nagios");
    default:
      break;
    }
    return gettext("Unknown");
  }

  function drawTableBody(rd) {
    var x;
    var s;
    var o;
    var ip, serverURL, mapsURL;

    s = "";
    for (x = 0; x < rd["servers"].length; ++x) {
      o = rd["servers"][x];
      ip = o["ipAddress"];

      var serverId = o["id"];
      serverIds.push(serverId);
      var idConnStat = getIdConnStat(serverId);
      serverURL = getServerLocation(o);
      mapsURL = getMapsLocation(o);
      s += "<tr>";
      s += "<td class='delete-selector' style='display:none;'>";
      s += "<input type='checkbox' class='selectcheckbox' serverId='" + escapeHTML(o["id"]) + "'>";
      s += "</td>";
      s += "<td id='"+ idConnStat + "' " +
             "data-title='" + gettext("Server ID") + ": " + serverId + "'" +
             "data-html=true " +
             "data-trigger='hover' " +
             "data-container='body' " +
           ">" + escapeHTML(gettext("Checking")) + "</td>";
      s += "<td>" + getServerTypeLabel(o["type"]) + "</td>";
      if (serverURL) {
        s += "<td><a href='" + serverURL + "'>" + escapeHTML(o["hostName"])  + "</a></td>";
        s += "<td><a href='" + serverURL + "'>" + escapeHTML(ip) + "</a></td>";
      } else {
        s += "<td>" + escapeHTML(o["hostName"])  + "</td>";
        s += "<td>" + escapeHTML(ip) + "</td>";
      }
      s += "<td>" + escapeHTML(o["nickname"])  + "</td>";
      if (mapsURL) {
        s += "<td><a href='" + mapsURL + "'>" + gettext('Show Maps') + "</a></td>";
      } else {
        s += "<td></td>";
      }
      s += "<td class='edit-server-column' style='display:none;'>";
      s += "<input id='edit-server" + escapeHTML(o["id"]) + "'";
      s += "  type='button' class='btn btn-default'";
      s += "  serverId='" + escapeHTML(o["id"]) + "'";
      s += "  value='" + gettext("EDIT") + "' />";
      s += "</td>";
      s += "</tr>";
    }

    return s;
  }

  function updateCore(reply) {
    $("#table tbody").empty();
    $("#table tbody").append(drawTableBody(reply));
    setupCheckboxHandler();
    setupEditButtons(reply);
    numSelected = 0;
    self.startConnection("server-conn-stat", updateServerConnStat);
  }

  function getIdConnStat(serverId) {
    var idConnStat = "connStat-" + serverId;
    return idConnStat;
  }

  function setStatusFailure(serverId) {
    var idConnStat = getIdConnStat(serverId);
    var label = gettext("Failed to get");
    $("#" + idConnStat).html(label);
  }

  function setStatusFailureAll() {
    for (var i = 0; i < serverIds.length; i++)
      setStatusFailure(serverIds[i]);
  }

  function updateServerConnStat(reply) {
    var connStatParser = new ServerConnStatParser(reply);
    if (connStatParser.isBadPacket()) {
        setStatusFailureAll();
        return;
    }
    for (var i = 0; i < serverIds.length; i++) {
      var serverId = serverIds[i];
      if (!connStatParser.setServerId(serverId)) {
        setStatusFailure(serverId);
        continue;
      }
      var label = connStatParser.getStatusLabel();
      var idConnStat = getIdConnStat(serverId);
      $("#" + idConnStat).html(label);
      options = {content: connStatParser.getInfoHTML()};
      $("#" + idConnStat).popover(options);
    }
  }
};

ServersView.prototype = Object.create(HatoholMonitoringView.prototype);
ServersView.prototype.constructor = ServersView;

var ServerConnStatParser = function(reply) {
  var self = this;
  self.badPacket = false;
  self.connStat = reply.serverConnStat;
  self.currServerId = null;
  self.currConnStat = null;

  if (!self.connStat) {
    self.badPacket = true;
    return;
  }
}

ServerConnStatParser.prototype.isBadPacket = function() {
    return this.badPacket;
}

ServerConnStatParser.prototype.setServerId = function(serverId) {
  var self = this;
  var key = serverId.toString();
  var connStat = self.connStat[key];
  if (!connStat)
    return false;
  self.currServerId = serverId;
  self.currConnStat = connStat;
  return true; 
}

ServerConnStatParser.prototype.getStatusLabel = function() {
  var self = this;
  if (self.currConnStat == undefined)
    throw new Error("Called before a valid server ID is set.");
  var currStatNum = self.currConnStat.status;
  if (currStatNum == undefined)
    return gettext("N/A");

  // TODO: Use a constant export from the server code.
  switch(currStatNum) {
  case hatohol.ARM_WORK_STAT_INIT:
    return gettext("Inital State");
  case hatohol.ARM_WORK_STAT_OK:
    return gettext("OK");
  case hatohol.ARM_WORK_STAT_FAILURE:
    return gettext("Error");
  default:
    return gettext("Unknown:") + currStatNum;
  }
  throw new Error("This line must no be executed.");
}

ServerConnStatParser.prototype.getInfoHTML = function() {
  var self = this;
  if (self.currConnStat == undefined)
    throw new Error("Called before a valid server ID is set.");
  var s = "";

  // running
  var running = self.currConnStat.running;
  s += gettext("Running") + ": " 
  switch (running) {
  case 0:
    s += gettext("No");
    break;
  case 1:
    s += gettext("Yes");
    break;
  default:
    s += "Unknown: " + running;
    break;
  }

  // status update time
  s += "<br>";
  var statUpdateTime = unixTimeToVisible(self.currConnStat.statUpdateTime);
  s += gettext("Status update time") + ": " + statUpdateTime;

  // last success time
  s += "<br>";
  var lastSuccessTime = unixTimeToVisible(self.currConnStat.lastSuccessTime);
  s += gettext("Last success time") + ": " + lastSuccessTime;

  // last failure time
  s += "<br>";
  var lastFailureTime = unixTimeToVisible(self.currConnStat.lastFailureTime);
  s += gettext("Last failure time") + ": " + lastFailureTime;

  // number of updates
  s += "<br>";
  var numUpdate = self.currConnStat.numUpdate;
  if (numUpdate == undefined)
    s += "N/A";
  else
    s += gettext("Number of communication") + ": " + numUpdate;

  // number of failures
  s += "<br>";
  var numFailure = self.currConnStat.numFailure;
  if (numFailure == undefined)
    s += "N/A";
  else
    s += gettext("Number of failure") + ": " + numFailure;

  // last failure comment
  s += "<br>";
  s += gettext("Comment for the failure") + ": ";
  var failureComment = self.currConnStat.failureComment;
  if (failureComment == undefined)
    s += "N/A";
  else if (failureComment == "")
    s += "-";
  else
    s += failureComment;

  function unixTimeToVisible(unixTimeString) {
    if (unixTimeString == undefined)
      return "N/A";
    var unixTime = Number(unixTimeString);
    if (unixTime == 0)
      return "-";
    var date = new Date(unixTime * 1000);
    return date.toLocaleString();
  }

  return s;
}
