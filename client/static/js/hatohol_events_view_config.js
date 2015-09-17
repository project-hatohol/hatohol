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
  var autoReloadInterval = 60;
  var minAutoReloadInterval = 5;
  var maxAutoReloadInterval = 600;
  var numRowsPerPage = 300;

  $("#auto-reload-interval-slider").slider({
    value: autoReloadInterval,
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

  $("#auto-reload-interval").val(autoReloadInterval);
  $("#num-rows-per-page").val(numRowsPerPage);
};
