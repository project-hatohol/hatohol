/*
 * Copyright (C) 2016 Project Hatohol
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

var HatoholTriggersViewConfig = function(options) {
  //
  // options has the following parameters.
  //
  // loadedCallback: <function> [optional]
  //   A callback function that is called on the loaded.
  //
  // savedCallback: <function> [optional]
  //   A callback function that is called on the saved.
  //
  var self = this;
  var defaultAutoReloadInterval = 60;
  var minAutoReloadInterval = 5;
  var maxAutoReloadInterval = 600;
  var defaultNumberOfRows = 300;
  var minNumberOfRows = 10;
  var maxNumberOfRows = 500;

  HatoholUserConfig.apply(this, [options]);
  self.options = options;
  self.config = self.getDefaultConfig();

  $('#triggers-view-config').on('show.bs.modal', function (event) {
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
    if (isNaN(value)) {
      value = defaultAutoReloadInterval;
      $("#auto-reload-interval").val(value);
    }

    $("#auto-reload-interval-slider").slider("value", value);
  });

  $("#num-rows-per-page").change(function() {
    var value = $("#num-rows-per-page").val();

    if (value < minNumberOfRows) {
      value = minNumberOfRows;
      $("#num-rows-per-page").val(value);
    }
    if (value > maxNumberOfRows) {
      value = maxNumberOfRows;
      $("#num-rows-per-page").val(value);
    }
    if (isNaN(value)) {
      value = defaultNumberOfRows;
      $("#num-rows-per-page").val(value);
    }
  });

  $("#config-save").click(function() {
    self.saveAll();
  });

  self.loadAll();
};

HatoholTriggersViewConfig.prototype = Object.create(HatoholUserConfig.prototype);
HatoholTriggersViewConfig.prototype.constructor = HatoholTriggersViewConfig;

HatoholTriggersViewConfig.prototype.loadAll = function() {
  var self = this;

  self.get({
    itemNames: Object.keys(self.getDefaultConfig()),
    successCallback: function(config) {
      $.extend(self.config, config);
      self.reset();
    },
    connectErrorCallback: function(XMLHttpRequest, textStatus, errorThrown) {
      self.showXHRError(XMLHttpRequest);
    },
    completionCallback: function() {
      if (self.options.loadedCallback)
        self.options.loadedCallback(self);
    },
  });
};

HatoholTriggersViewConfig.prototype.saveAll = function() {
  var self = this;

  $.extend(self.config, {
    'triggers.auto-reload.interval': $("#auto-reload-interval").val(),
    'triggers.num-rows-per-page': $("#num-rows-per-page").val(),
  });

  self.store({
    items: self.config,
    successCallback: function(reply) {
      $("#triggers-view-config").modal("hide");
    },
    connectErrorCallback: function(XMLHttpRequest, textStatus, errorThrown) {
      self.showXHRError(XMLHttpRequest);
    },
    completionCallback: function() {
      if (self.options.savedCallback)
        self.options.savedCallback(self);
    },
  });
};

HatoholTriggersViewConfig.prototype.getValue = function(key) {
    var defaultConfig = this.getDefaultConfig();
    return this.findOrDefault(this.config, key, defaultConfig[key]);
};

HatoholTriggersViewConfig.prototype.reset = function() {
  var self = this;
  var autoReloadInterval = self.getValue('triggers.auto-reload.interval');

  resetAutoReloadInterval();
  resetNumRowsPerPage();

  function resetAutoReloadInterval() {
    $("#auto-reload-interval-slider").slider("value", autoReloadInterval);
    $("#auto-reload-interval").val(autoReloadInterval);
  }

  function resetNumRowsPerPage() {
    $("#num-rows-per-page").val(self.getValue('triggers.num-rows-per-page'));
  }
};

HatoholTriggersViewConfig.prototype.getDefaultConfig = function() {
  return {
    'triggers.auto-reload.interval': "60",
    'triggers.num-rows-per-page': "50",
  };
};

HatoholTriggersViewConfig.prototype.showXHRError = function (XMLHttpRequest) {
  var errorMsg = "Error: " + XMLHttpRequest.status + ": " +
    XMLHttpRequest.statusText;
  hatoholErrorMsgBox(errorMsg);
};
