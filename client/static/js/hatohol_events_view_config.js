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

var HatoholEventsViewConfig = function(options) {
  var self = this;
  var minAutoReloadInterval = 5;
  var maxAutoReloadInterval = 600;

  HatoholUserConfig.apply(this, options);
  self.config = self.getDefaultConfig();

  $('#events-view-config').on('hidden.bs.modal', function (event) {
    self.reset();
  });

  $("#auto-reload-interval-slider").slider({
    min: minAutoReloadInterval,
    max: maxAutoReloadInterval,
    step: 1,
    slide: function(event, ui) {
      $("#auto-reload-interval").val(ui.value);
    },
  });

  $("#auto-reload-interval").change(function() {
    var value = $("#auto-reload-interval").val();

    if (value < minAutoReloadInterval) {
      value = minAutoReloadInterval;
      $("#auto-reload-interval").val(value);
    }
    if (value > maxAutoReloadInterval) {
      value = maxAutoReloadInterval;
      $("#auto-reload-interval").val(value);
    }

    $("#auto-reload-interval-slider").slider("value", value);
  });

  $("#config-save").click(function() {
    self.saveAll();
  });

  self.loadAll();
};

HatoholEventsViewConfig.prototype = Object.create(HatoholUserConfig.prototype);
HatoholEventsViewConfig.prototype.constructor = HatoholEventsViewConfig;

HatoholEventsViewConfig.prototype.loadAll = function() {
  var self = this;
  self.get({
    itemNames: [
      'events.auto-reload.interval',
      'events.num-rows-per-page',
      'events.sort.type',
      'events.sort.order',
    ],
    successCallback: function(config) {
      $.extend(self.config, config);
      self.reset();
    },
    connectErrorCallback: function(XMLHttpRequest, textStatus, errorThrown) {
      self.showXHRError(XMLHttpRequest);
    },
  })
}

HatoholEventsViewConfig.prototype.saveAll = function() {
  var self = this;
  $.extend(self.config, {
    'events.auto-reload.interval': $("#auto-reload-interval").val(),
    'events.num-rows-per-page': $("#num-rows-per-page").val(),
  })
  self.store({
    items: self.config,
    successCallback: function(reply) {
      $("#events-view-config").modal("hide");
    },
    connectErrorCallback: function(XMLHttpRequest, textStatus, errorThrown) {
      self.showXHRError(XMLHttpRequest);
    },
  });
}

HatoholEventsViewConfig.prototype.reset = function() {
  var self = this;
  var autoReloadInterval = self.config['events.auto-reload.interval']
  $("#auto-reload-interval-slider").slider("value", autoReloadInterval);
  $("#auto-reload-interval").val(autoReloadInterval);
  $("#num-rows-per-page").val(self.config['events.num-rows-per-page']);
}

HatoholEventsViewConfig.prototype.getDefaultConfig = function() {
  return {
    'events.auto-reload.interval': 60,
    'events.num-rows-per-page': 300,
    'events.columns':
      "incidentStatus,status,severity,time," +
      "monitoringServerName,hostName,description",
    'events.sort.type': "time",
    'events.sort.order': hatohol.DATA_QUERY_OPTION_SORT_DESCENDING,
  }
}

HatoholEventsViewConfig.prototype.showXHRError = function (XMLHttpRequest) {
  var errorMsg = "Error: " + XMLHttpRequest.status + ": " +
    XMLHttpRequest.statusText;
  hatoholErrorMsgBox(errorMsg);
}
