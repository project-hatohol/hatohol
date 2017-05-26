/*
 * Copyright (C) 2014 Project Hatohol
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

var LogSearchSystemsView = function(userProfile) {
  //
  // Variables
  //
  var self = this;

  // call the constructor of the super class
  HatoholMonitoringView.apply(this, [userProfile]);

  //
  // main code
  //
  $("#delete-log-search-system-button").show();
  load();

  //
  // Main view
  //
  $("#add-log-search-system-button").click(function() {
    new HatoholLogSearchSystemEditor({
      succeededCallback: load
    });
  });

  $("#delete-log-search-system-button").click(function() {
    var msg = gettext("Delete the selected items ?");
    hatoholNoYesMsgBox(msg, deleteLogSearchSystems);
  });

  function setupEditButtons(logSearchSystems) {
    logSearchSystems.forEach(function(logSearchSystem) {
      var elementID = "#edit-log-search-system-" + logSearchSystem.id;
      $(elementID).click(function() {
        new HatoholLogSearchSystemEditor({
          logSearchSystem: logSearchSystem,
          succeededCallback: load
        });
      });
    });
  }

  //
  // Commonly used functions from a dialog.
  //
  function HatoholLogSearchSystemReplyParser(data) {
    this.data = data;
  }

  HatoholLogSearchSystemReplyParser.prototype.getStatus = function() {
    console.log("Here");
    return REPLY_STATUS.OK;
  };

  function deleteLogSearchSystems() {
    $(this).dialog("close");
    var checkboxes = $(".selectcheckbox");
    var deleteList = [];
    var i;
    for (i = 0; i < checkboxes.length; i++) {
      if (checkboxes[i].checked)
        deleteList.push(checkboxes[i].getAttribute("logSearchSystemID"));
    }
    new HatoholItemRemover({
      id: deleteList,
      type: "log-search-systems",
      completionCallback: function() {
        load();
      }
    }, {
      pathPrefix: '',
      replyParser: HatoholLogSearchSystemReplyParser
    });
    hatoholInfoMsgBox(gettext("Deleting..."));
  }

  //
  // callback function from the base template
  //
  function drawTableBody(logSearchSystems) {
    var table = "";
    logSearchSystems.forEach(function(logSearchSystem) {
      table += "<tr>";
      table += "<td class='delete-selector' style='display:none'>";
      table += "<input type='checkbox' class='selectcheckbox' " +
        "logSearchSystemID='" + escapeHTML(logSearchSystem.id) + "'></td>";
      table += "<td>" + escapeHTML(logSearchSystem.id) + "</td>";
      table += "<td>" + escapeHTML(logSearchSystem.type) + "</td>";
      table += "<td>" + escapeHTML(logSearchSystem.base_url) + "</td>";
      table += "<td class='edit-log-search-system-column' style='display:none'>";
      table += "<input type='button' class='btn btn-default' " +
        "logSearchSystemID='" + escapeHTML(logSearchSystem.id) + "' " +
        "value='" + gettext("EDIT") + "' " +
        "id='edit-log-search-system-" + escapeHTML(logSearchSystem.id) + "' />";
      table += "</td>";
      table += "</tr>";
    });
    return table;
  }

  function onGotLogSearchSystems(logSearchSystems) {
    self.logSearchSystems = logSearchSystems;
    $("#table tbody").empty();
    $("#table tbody").append(drawTableBody(self.logSearchSystems));
    self.setupCheckboxForDelete($("#delete-log-search-system-button"));
    $(".delete-selector").show();
    setupEditButtons(self.logSearchSystems);
    $(".edit-log-search-system-column").show();
  }

  function load() {
    self.displayUpdateTime();
    self.startConnection('log-search-systems/',
                         onGotLogSearchSystems,
                         null,
                         {pathPrefix: ''});
  }
};

LogSearchSystemsView.prototype = Object.create(HatoholMonitoringView.prototype);
LogSearchSystemsView.prototype.constructor = LogSearchSystemsView;
