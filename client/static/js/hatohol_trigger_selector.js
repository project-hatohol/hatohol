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
  self.queryData = {"serverId": serverId, "hostId": hostId};

  // call the constructor of the super class
  HatoholSelectorDialog.apply(
    this, ["trigger-selector", gettext("Trigger selecion"), selectedCb]);
  self.start("/tunnel/trigger", "GET");
}

HatoholTriggerSelector.prototype =
  Object.create(HatoholSelectorDialog.prototype);
HatoholTriggerSelector.prototype.constructor = HatoholTriggerSelector;

HatoholTriggerSelector.prototype.makeQueryData = function() {
    return this.queryData;
}

HatoholTriggerSelector.prototype.getNumberOfObjects = function(reply) {
  return reply.numberOfTriggers;
}

HatoholTriggerSelector.prototype.generateMainTable = function(tableId) {
  var html =
  '<table class="table table-condensed table-striped table-hover" id=' +
  tableId + '>' +
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

HatoholTriggerSelector.prototype.generateTableRows = function(reply) {
  var s = "";
  this.setObjectArray(reply.triggers);
  for (var i = 0; i < reply.triggers.length; i++) {
    trigger = reply.triggers[i];
    s += '<tr>';
    s += '<td>' + trigger.id + '</td>';
    s += '<td>' + trigger.brief + '</td>';
    s += '</tr>';
  }
  return s;
}
