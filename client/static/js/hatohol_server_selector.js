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

var HatoholServerSelector = function(selectedCb) {

    var dialogButtons = [{
      text: gettext("SELECT"),
      click: function() {
        var ctx = getContext();
        if (selectedCb) {
          if (!ctx.selectedRow)
            selectedCb(null);
          else
            selectedCb(ctx.serverArray[ctx.selectedRow.index()]);
        }
        $(this).dialog("close");
        $("#server-selector").remove();
      }
    }, {
      text: gettext("CANCEL"),
      click: function() {
        if (selectedCb)
          selectedCb(null);
        $(this).dialog("close");
        $("#server-selector").remove();
      }
    }];

    var div = $("<div>");
    div.attr("id", "serverSelectMainDiv");
    $("body").append(div);
    var ctx = {
      selectedRow: null,
      serverArray: null,
    };
    $("#serverSelectMainDiv").data("ctx", ctx);

    HatoholDialog("server-selector", "Server selecion",
                  div, dialogButtons);
    setSelectButtonState(false);
    showInitialView();
    getServerList();

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

    function showInitialView() {
      $("#serverSelectMainDiv").html(
        '<p id="serverSelectMsgArea">' + gettext("Now getting servers...") +
        '</p>');
    }

    function makeTable(data) {
      var html =
      '<table class="table table-condensed table-striped table-hover" id="serverSelectTable">' +
      '  <thead>' +
      '    <tr>' +
      '      <th>ID</th>' +
      '      <th>' + gettext("Type") + '</th>' +
      '      <th>' + gettext("Hostname") + '</th>' +
      '      <th>' + gettext("IP Address") + '</th>' +
      '      <th>' + gettext("Nickname") + '</th>' +
      '    </tr>' +
      '  </thead>' +
      '  <tbody></tbody>' +
      '</table>'
      return html;
    }

    function getContext() {
      return $("#serverSelectMainDiv").data("ctx");
    }

    function makeTableBody(reply) {
      var s;
      var ctx = getContext();
      ctx.serverArray = reply.servers;
      for (var i = 0; i < reply.servers.length; i++) {
        sv = reply.servers[i];
        s += '<tr>';
        s += '<td>' + sv.id + '</td>';
        s += '<td>' + sv.type + '</td>';
        s += '<td>' + sv.hostName + '</td>';
        s += '<td>' + sv.ipAddress + '</td>';
        s += '<td>' + sv.nickname  + '</td>';
        s += '</tr>';
      }
      return s;
    }

    function getServerList() {
      $.ajax({
        url: "/tunnel/server",
        type: "GET",
        success: function(reply) {
          var replyParser = new HatoholReplyParser(reply);
          if (!(replyParser.getStatus() === REPLY_STATUS.OK)) {
            $("#serverSelectMainDiv").text(replyParser.getStatusMessage());
            return;
          }
          $("#serverSelectMainDiv").html(makeTable(reply));
          $("#serverSelectMainDiv tbody").empty();
          $("#serverSelectmainDiv tbody").append(makeTableBody(reply));
          $("#serverSelectTable tr").click(function(){
            var ctx = getContext();
            if (ctx.selectedRow)
              ctx.selectedRow.removeClass("info");
            else
              setSelectButtonState(true);
            $(this).attr("class", "info");
            ctx.selectedRow = $(this);
          });
        },
        error: function(XMLHttpRequest, textStatus, errorThrown) {
          var errorMsg = "Error: " + XMLHttpRequest.status + ": " +
                         XMLHttpRequest.statusText;
          $("#serverSelectMainDiv").text(errorMsg);
        }
      })
    }
}
