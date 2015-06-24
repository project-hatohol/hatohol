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

Array.prototype.uniq = function() {
  var o = {}, i, len = this.length, r = [];
  for (i = 0; i < len; ++i) o[this[i]] = true;
  for (i in o) r.push(i);
  return r;
};

function padDigit(val, len) {
  var s = "00000000" + val;
  return s.substr(-len);
}

function formatDate(str) {
  if (!str)
    return "-";
  var val = new Date();
  val.setTime(Number(str) * 1000);
  var d =
    val.getFullYear() + "/" +
    padDigit(val.getMonth() + 1, 2) + "/" +
    padDigit(val.getDate(), 2);
  var t =
    padDigit(val.getHours(), 2) + ":" +
    padDigit(val.getMinutes(), 2) + ":" +
    padDigit(val.getSeconds(), 2);
  return d + " " + t;
}

function formatSecond(sec) {
  var t =
    padDigit(parseInt(sec / 3600), 2) + ":" +
    padDigit(parseInt(sec / 60) % 60, 2) + ":" +
    padDigit(sec % 60, 2);
  return t;
}

function makeTriggerStatusLabel(status) {
  switch(status) {
  case TRIGGER_STATUS_OK:
    return gettext("OK");
  case TRIGGER_STATUS_PROBLEM:
    return gettext("Problem");
  default:
    return "INVALID: " + status;
  }
}

function makeSeverityLabel(severity) {
  switch(severity) {
  case TRIGGER_SEVERITY_UNKNOWN:
    return gettext("Not classified");
  case TRIGGER_SEVERITY_INFO:
    return gettext("Information");
  case TRIGGER_SEVERITY_WARNING:
    return gettext("Warning");
  case TRIGGER_SEVERITY_ERROR:
    return gettext("Average");
  case TRIGGER_SEVERITY_CRITICAL:
    return gettext("High");
  case TRIGGER_SEVERITY_EMERGENCY:
    return gettext("Disaster");
  default:
    return "INVALID: " + severity;
  }
}

function makeMonitoringSystemTypeLabel(server) {
  var type = getServerTypeId(server);
  var pluginName;

  if (type == null || type == undefined)
    return "Invalid";

  pluginName = "hap_" + type;
  if (!hatohol[pluginName])
    return "Invalid: " + type;

  return hatohol[pluginName].label;
}

function isIPv4(ipAddress) {
  var ipv4Regexp = /^([0-9]{1,3})\.([0-9]{1,3})\.([0-9]{1,3})\.([0-9]{1,3})$/;
  return ipAddress.match(ipv4Regexp);
}

function getServerTypeId(server) {
  if (!server)
    return null;
  if (server.type == hatohol.MONITORING_SYSTEM_HAPI2)
      return server.uuid;
  return server.type;
}

function getPlugin(server) {
  var type = getServerTypeId(server);

  if (type == null || type == undefined)
    return undefined;

  return hatohol["hap_" + type];
}

function getServerLocation(server) {
  var plugin = getPlugin(server);
  if (!plugin || !plugin.getTopURL)
    return undefined;
  return plugin.getTopURL(server);
}

function getMapsLocation(server) {
  var plugin = getPlugin(server);
  if (!plugin || !plugin.getMapsURL)
    return undefined;
  return plugin.getMapsURL(server);
}

function getItemGraphLocation(server, itemId) {
  var plugin = getPlugin(server);
  if (!plugin || !plugin.getItemGraphURL)
    return undefined;
  return plugin.getItemGraphURL(server, itemId);
}

function getServerName(server, serverId) {
  if (!server || !server["name"])
    return gettext("Unknown") + " (ID: " + serverId + ")";
  return server["name"];
}

function getNickName(server, serverId) {
  if (!server || !server["nickname"])
    return gettext("Unknown") + " (ID: " + serverId + ")";
  return server["nickname"];
}

function getHostgroupName(server, hostgroupId) {
  var getNamelessHostgroupName = function(hostgroupId) {
    return gettext("Unknown") + " (ID: " + hostgroupId + ")";
  };

  if (!server || !server["groups"] || !(hostgroupId in server["groups"]))
    return getNamelessHostgroupName(hostgroupId);

  var hostgroupName = server["groups"][hostgroupId]["name"];
  if (!hostgroupName)
    return getNamelessHostgroupName(hostgroupId);

  return hostgroupName;
}

function getHostName(server, hostId) {
  var getNamelessHostName = function(hostId) {
    return gettext("Unknown") + " (ID: " + hostId + ")";
  };

  if (!server || !server["hosts"] || !(hostId in server["hosts"]))
    return getNamelessHostName(hostId);

  var hostName = server["hosts"][hostId]["name"];
  if (!hostName)
    return getNamelessHostName(hostId);

  return hostName;
}

function getTriggerBrief(server, triggerId) {
  var getNamelessTriggerName = function(triggerId) {
    return gettext("Unknown") + " (ID: " + triggerId + ")";
  };

  if (!server || !server["triggers"] || !(triggerId in server["triggers"]))
    return getNamelessTriggerName(triggerId);

  var triggerName = server["triggers"][triggerId]["brief"];
  if (!triggerName)
    return getNamelessTriggerName(triggerId);

  return triggerName;
}

function getItemBriefWithUnit(item) {
  var label = item.brief;
  if (item.unit)
    label += " [" + item.unit + "]";
  return label;
}

function escapeHTML(html) {
  return $('<div/>').text(html).html();
};

function deparam(query) {
  var offset;

  if (!query) {
    offset = window.location.href.indexOf('?');
    if (offset < 0)
      return {};
    query = window.location.href.slice(offset + 1);
  }

  return $.deparam(query);
};

function getMetricPrefix(pow) {
  switch(pow) {
  case 0:
    return "";
  case 1:
    return "K";
  case 2:
    return "M";
  case 3:
    return "G";
  case 4:
    return "T";
  case 5:
    return "P";
  case 6:
    return "E";
  default:
    return "";
  }
}

function formatMetricPrefix(value, unit, step, pow, digits) {
  var text, maxPow = 6, unit;
  var blackList = {
    '%': true,
    'ms': true,
    'rpm': true,
    'RPM': true,
  };

  if (isNaN(value))
    return escapeHTML(value);

  if (!step)
    step = 1000;

  if (!pow) {
    if (!unit || (unit in blackList) || (value < step))
      pow = 0;
    else
      pow = Math.floor(Math.log(value) / Math.log(step));
  }
  if (pow < 0)
    pow = 0;
  if (pow > maxPow)
    pow = maxPow;

  if (!digits)
    digits = 4;

  if (pow == 0 && value.match(/^-?\d+$/))
    return value + " " + escapeHTML(unit);

  text = value / Math.pow(step, pow);
  text = text.toPrecision(digits).replace(/(?:\.0+|(\.\d+?)0+)$/, "$1");
  unit = getMetricPrefix(pow) + escapeHTML(unit);
  if (unit)
    text += " " + unit;
  return text;
}

function formatUptime(seconds) {
  var secondsPerDay = 60 * 60 * 24;
  var days = Math.floor(seconds / secondsPerDay);
  var text = "";

  seconds = seconds - secondsPerDay * days;

  if (days == 1)
    text += days + gettext(" day, ");
  else if (days > 0)
    text += days + gettext(" days, ");
  text += formatSecond(seconds);

  return text;
}

function formatItemValue(value, unit) {
  if (isNaN(value))
    return escapeHTML(value);

  if (!unit) {
    if (value.match(/^-?\d+$/)) {
      // integer
      return escapeHTML(value);
    } else {
      // float: format digits
      return formatMetricPrefix(value, unit, 1000, 0, 4);
    }
  }

  if (unit == "unixtime")
    return formatDate(value);

  if (unit == "uptime")
    return formatUptime(value);

  if (unit == 'B' || unit == 'Bps')
    return formatMetricPrefix(value, unit, 1024);
  else
    return formatMetricPrefix(value, unit);
};

function isInt(value) {
  // Caution: It doesn't work correctly against a number like 1.0.
  return !isNaN(value) && !isFloat(value);
}

function isFloat(value) {
  // Caution: It doesn't work correctly against a number like 1.0.
  return !isNaN(value) && value.toString().indexOf('.') != -1;
}

function formatItemLastValue(item) {
  if (isNaN(item["lastValue"]))
    return escapeHTML(item["lastValue"]);
  return formatItemValue(item["lastValue"], item["unit"]);
}

function formatItemPrevValue(item) {
  if (isNaN(item["prevValue"]))
    return escapeHTML(item["prevValue"]);
  return formatItemValue(item["prevValue"], item["unit"]);
}

(function(global) {
  function Namespace(str) {
    var spaces = str.split('.');
    var i, here = global;
    for (i = 0; i < spaces.length; i++) {
      if (typeof(here[spaces[i]]) == 'undefined')
        here[spaces[i]] = {};
      here = here[spaces[i]];
    }
    return here;
  }

  var hatohol = Namespace("hatohol");
  hatohol.addNamespace = Namespace;
  hatohol.isIPv4 = isIPv4;
  hatohol.escapeHTML = escapeHTML;
}(this));
