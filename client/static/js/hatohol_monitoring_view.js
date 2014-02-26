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
};

HatoholMonitoringView.prototype.setCandidate = function(target, list) {
  var x;
  var html = "<option>---------</option>";

  target.empty().append(html);

  if (list) {
    target.removeAttr("disabled");
    for (x = 0; list && x < list.length; ++x) {
      var option = $("<option/>");
      option.text(list[x]);
      target.append(option);
    }
  } else {
    target.attr("disabled", "disabled");
  }
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
  function (tableName, completionCallback, callbackParam)
{
  var self = this;
  
  self.setStatus({
    "class" : "warning",
    "label" : gettext("LOAD"),
    "lines" : [ gettext("Communicating with backend") ],
  });

  var connParam =  {
    url: '/' + tableName,
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
  new HatoholConnector(connParam);
};
