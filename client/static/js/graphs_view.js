/*
 * Copyright (C) 2015 Project Hatohol
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

var GraphsView = function(userProfile) {
  //
  // Variables
  //
  var self = this;

  // call the constructor of the super class
  HatoholMonitoringView.apply(this, [userProfile]);

  //
  // main code
  //
  $("#delete-graph-button").show();
  load();

  //
  // Main view
  //
  $("#delete-graph-button").click(function() {
    var msg = gettext("Do you delete the selected items ?");
    hatoholNoYesMsgBox(msg, deleteGraphs);
  });

  //
  // Commonly used functions from a dialog.
  //
  function HatoholGraphReplyParser(data) {
    this.data = data;
  }

  HatoholGraphReplyParser.prototype.getStatus = function() {
    return REPLY_STATUS.OK;
  };

  function deleteGraphs() {
    $(this).dialog("close");
    var checkboxes = $(".selectcheckbox");
    var deleteList = [];
    var i;
    for (i = 0; i < checkboxes.length; i++) {
      if (checkboxes[i].checked)
        deleteList.push(checkboxes[i].getAttribute("graphID"));
    }
    new HatoholItemRemover({
      id: deleteList,
      type: "graphs",
      completionCallback: function() {
        load();
      }
    }, {
      pathPrefix: '',
      replyParser: HatoholGraphReplyParser
    });
    hatoholInfoMsgBox(gettext("Deleting..."));
  }

  //
  // callback function from the base template
  //
  function drawTableBody(graphs) {
    var table = "";
    for (let graph of graphs) {
      var title = graph.title ? escapeHTML(graph.title) : gettext("No title");
      var graphID = escapeHTML(graph.id);
      var graphURL = "ajax_history?id=" + graphID;
      table += "<tr>";
      table += "<td class='delete-selector' style='display:none'>";
      table += "<input class='selectcheckbox'" +
               "  value='" + graphs.indexOf(graph) + "'" +
               "  graphID='" + graphID + "' type='checkbox'></td>";
      table += "<td>" + graphID + "</td>";
      table += "<td>" +  '<a target="_blank" href="' + graphURL + '">' +
               title + '</a></td>';

      var editURL = "ajax_history?mode=edit&id=" + graphID;
      table += "<td>" + anchorTagForDomesticLink(editURL, gettext("EDIT"),
                                                 "btn btn-default") + "</td>";
      table += "</tr>";
    };
    return table;
  }

  function onGotGraphs(graphs) {
    self.graphs = graphs;
    $("#table tbody").empty();
    $("#table tbody").append(drawTableBody(self.graphs));
    self.setupCheckboxForDelete($("#delete-graph-button"));
    $(".delete-selector").shiftcheckbox();
    $(".delete-selector").show();
    $(".edit-graph-column").show();
  }

  function load() {
    self.displayUpdateTime();
    self.startConnection('graphs/',
                         onGotGraphs,
                         null,
                         {pathPrefix: ''});
  }
};

GraphsView.prototype = Object.create(HatoholMonitoringView.prototype);
GraphsView.prototype.constructor = GraphsView;
