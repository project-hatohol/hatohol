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

var HatoholItemSelector = function(options) {
  var self = this;

  self.options = options || {};
  self.lastIndex = 0;
  self.elementId = 'hatohol-item-list';
  self.servers = self.options.servers;
  self.lastItemsData = undefined;
  self.rowData = {};
  self.view = self.options.view; // TODO: Remove view dependency
  self.appendItemCallback = self.options.appendItemCallback;
  self.removeItemCallback = self.options.removeItemCallback;

  setup();

  function setup() {
    self.view.setupHostQuerySelectorCallback(
      function() {
        var query = self.view.getHostFilterQuery();
        self.view.startConnection("item?empty=true&" + $.param(query), function(reply) {
          self.servers = reply.servers;
          self.setupCandidates();
        });
      },
      '#select-server', '#select-host-group', '#select-host');
    $("#select-item").attr("disabled", "disabled");
    $("#add-item-button").attr("disabled", "disabled");
    $("#select-server").change(resetItemField);
    $("#select-host-group").change(resetItemField);
    $("#select-host").change(loadItemCandidates);
    $("#select-item").change(function() {
      if ($(this).val() == "---------")
        $("#add-item-button").attr("disabled", "disabled");
      else
        $("#add-item-button").removeAttr("disabled");
    });
    $("#add-item-button").click(function() {
      var query = self.view.getHostFilterQuery();
      var i, index, item;

      query.itemId = $("#select-item").val();

      for (i = 0; i < self.lastItemsData.items.length; i++) {
        if (self.lastItemsData.items[i].id == query.itemId)
          item = self.lastItemsData.items[i];
      }

      index = self.appendItem(item, self.lastItemsData.servers,
                              query.hostgroupId);
      setItemCandidates(self.lastItemsData);
      $("#add-item-button").attr("disabled", "disabled");

      if (self.appendItemCallback)
        self.appendItemCallback(index, query);
    });
    setupPopover("#select-item");
  }

  function isAlreadyAddedItem(item1) {
    var i, item2;
    for (i in self.rowData) {
      item2 = self.rowData[i].item;
      if (!item2)
        continue;
      if (item1.serverId == item2.serverId &&
          item1.hostId == item2.hostId &&
          item1.id == item2.id)
        return true;
    }
    return false;
  }

  function setItemCandidates(reply) {
    var candidates = $.map(reply.items, function(item) {
      if (isAlreadyAddedItem(item))
        return null;
      var label = getItemBriefWithUnit(item);
      return { label: label, value: item.id };
    });
    self.view.setFilterCandidates($("#select-item"), candidates);
    self.lastItemsData = reply;
  }

  function loadItemCandidates() {
    var query;
    var hostName = $("#select-host").val();

    $("#add-item-button").attr("disabled", "disabled");

    if (hostName == "---------") {
      self.view.setFilterCandidates($("#select-item"));
      return;
    }

    query = self.view.getHostFilterQuery();
    self.view.startConnection("items?" + $.param(query), setItemCandidates);
  }

  function setupPopover(selector) {
    $(selector).popover({
      content: function() {
        var selectedItem = $(selector).children(':selected').text();
        return selectedItem;
      }
    }).on("mouseenter", function () {
      $(selector).popover("show");
    }).on("mouseleave", function () {
      $(selector).popover("hide");
    });
  }

  function resetItemField() {
    self.view.setFilterCandidates($("#select-item"));
  }
};

HatoholItemSelector.prototype.show = function() {
  var self = this;
  if (!self.servers) {
    self.view.startConnection("item?empty=true", function(reply) {
      self.servers = reply.servers;
      self.setupCandidates();
    });
  }
  $('#' + self.elementId).modal('show');
};

HatoholItemSelector.prototype.hide = function() {
  $('#' + this.elementId).modal('hide');
};

HatoholItemSelector.prototype.appendItem = function(item, servers, hostgroupId)
{
  return this.setItem(undefined, item, servers, hostgroupId);
};

HatoholItemSelector.prototype.setItem = function(index, item, servers,
                                                 hostgroupId)
{
  var self = this;
  var server = item ? servers[item.serverId] : undefined;
  var serverName = item ? getNickName(server, item.serverId) : "-";
  var hostName = item ? getHostName(server, item.hostId) : "-";
  var groupName = (hostgroupId && hostgroupId != "*") ?
    getHostgroupName(server, hostgroupId) : "-";
  var itemName = item ? getItemBriefWithUnit(item)  : "-";
  var id, tr;

  if (isNaN(index))
    index = self.lastIndex++;
  id = self.elementId + "-row-" + index;

  if (index in self.rowData) {
    tr = $("#" + id);
    tr.empty();
  } else {
    tr = $("<tr>", { id: id });
  }

  tr.append($("<td>"));
  tr.append($("<td>", { text: serverName }));
  tr.append($("<td>", { text: groupName }));
  tr.append($("<td>", { text: hostName }));
  tr.append($("<td>", { text: itemName }));
  tr.append($("<td>").append(
    $("<button>", {
      text: gettext("DELETE"),
      type: "button",
      "class": "btn btn-default",
      click: function() {
        var index = $(this).attr("itemIndex");
        $(this).parent().parent().remove();
        if (self.removeItemCallback)
          self.removeItemCallback(index);
        delete self.rowData[index];
      }
    }).attr("itemIndex", index)));

  if (!(index in self.rowData)) {
    self.rowData[index] = {};
    tr.insertBefore("#item-chooser-row");
  }

  if (item) {
    self.rowData[index] = self.rowData[index] || {};
    self.rowData[index].item = item;
    self.rowData[index].hostgroupId = hostgroupId;
  }

  return index;
};

HatoholItemSelector.prototype.setUserData = function(index, data) {
  this.rowData[index] = this.rowData[index] || {};
  this.rowData[index].userData = data;
};

HatoholItemSelector.prototype.getUserData = function(index) {
  if (this.rowData[index])
    return this.rowData[index].userData;
  else
    return undefined;
};

HatoholItemSelector.prototype.getIndexByUserData = function(data) {
  var index;
  for (index in this.rowData) {
    if (this.rowData[index] && this.rowData[index].userData == data)
      return parseInt(index);
  }
  return -1;
};

HatoholItemSelector.prototype.setupCandidates = function() {
  this.view.setServerFilterCandidates(this.servers);
  this.view.setHostgroupFilterCandidates(this.servers);
  this.view.setHostFilterCandidates(this.servers);
};

HatoholItemSelector.prototype.setServers = function(servers) {
  if (!this.servers)
    this.servers = servers;
  this.view.setupHostFilters(servers);
};

HatoholItemSelector.prototype.getConfig = function() {
  var i, item, data, config = { histories: [] };
  for (i in this.rowData) {
    item = this.rowData[i].item;
    data = {
      serverId: item.serverId,
      hostId: item.hostId,
      hostgroupId: this.rowData[i].hostgroupId,
      itemId: item.id
    };
    config.histories.push(data);
  }
  return config;
};
