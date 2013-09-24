/*
 * Copyright (C) 2013 Project Hatohol
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

var HatoholTriggerSelector = function(serverId, hostId, selectedCb) {
  var self = this;

  var dialogButtons = [{
    text: gettext("SELECT"),
    click: selectButtonClickedCb,
  }, {
    text: gettext("CANCEL"),
    click: cancelButtonClickedCb,
  }];

  // call the constructor of the super class
  HatoholDialog.apply(
    this, ["trigger-selector", gettext("Trigger selecion"), dialogButtons]);
  setSelectButtonState(false);
  getTriggerList();

  function selectButtonClickedCb() {
    if (selectedCb) {
      if (!self.selectedRow)
        selectedCb(null);
      else
        selectedCb(self.triggerArray[self.selectedRow.index()]);
    }
    self.closeDialog();
  }

  function cancelButtonClickedCb() {
    if (selectedCb)
      selectedCb(null);
    self.closeDialog();
  }

  function setSelectButtonState(state) {
    var btn = $(".ui-dialog-buttonpane").find("button:contains(" +
                gettext("SELECT") + ")");
    if (state) {
       btn.removeAttr("disabled");
       btn.removeClass("ui-state-disabled");
    } else {
       btn.attr("disabled", "disable");
       btn.addClass("ui-state-disabled");
    }
  }

  function makeTable(data) {
    var html =
    '<table class="table table-condensed table-striped table-hover" id="triggerSelectTable">' +
    '  <thead>' +
    '    <tr>' +
    '      <th>ID</th>' +
    '      <th>' + gettext("Brief") + '</th>' +
    '    </tr>' +
    '  </thead>' +
    '  <tbody></tbody>' +
    '</table>'
    return html;
  }

  function makeTableBody(reply) {
    var s;
    self.triggerArray = reply.triggers;
    for (var i = 0; i < reply.triggers.length; i++) {
      trigger = reply.triggers[i];
      s += '<tr>';
      s += '<td>' + trigger.id + '</td>';
      s += '<td>' + trigger.brief + '</td>';
      s += '</tr>';
    }
    return s;
  }

  function getTriggerList() {
    $.ajax({
      url: "/tunnel/trigger",
      type: "GET",
      data: {"serverId": serverId, "hostId": hostId},
      success: function(reply) {
        var replyParser = new HatoholReplyParser(reply);
        if (!(replyParser.getStatus() === REPLY_STATUS.OK)) {
          $("#triggerSelectMsgArea").text(replyParser.getStatusMessage());
          return;
        }
        if (reply.numberOfTriggers == 0) {
          $("#triggerSelectMsgArea").text(gettext("No data."));
          return;
        }

        // create a table
        var table = $(makeTable(reply));
        self.replaceMainElement(table);
        $("#triggerSelectTable tbody").empty();
        $("#triggerSelectTable tbody").append(makeTableBody(reply));

        // set events
        $("#triggerSelectTable tr").click(function(){
          if (self.selectedRow)
            self.selectedRow.removeClass("info");
          else
            setSelectButtonState(true);
          $(this).attr("class", "info");
          self.selectedRow = $(this);
        });
      },
      error: function(XMLHttpRequest, textStatus, errorThrown) {
        var errorMsg = "Error: " + XMLHttpRequest.status + ": " +
                       XMLHttpRequest.statusText;
        $("#triggerSelectMsgArea").text(errorMsg);
      }
    })
  }
}

HatoholTriggerSelector.prototype = Object.create(HatoholDialog.prototype);
HatoholTriggerSelector.prototype.constructor = HatoholTriggerSelector;

HatoholTriggerSelector.prototype.createMainElement = function() {
  var ptag = $("<p/>");
  ptag.attr("id", "triggerSelectMsgArea");
  ptag.text(gettext("Now getting trigger information..."));
  return ptag;
}

