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

var HatoholBootstrapDialog = function(id, dialogTitle, buttons, dialogAttr) {
  var self = this;
};

/**
 * Replace a main element of the dialog.
 * Note that the old element is deleted.
 *
 * @param elem A new element to be set.
 */
HatoholBootstrapDialog.prototype.replaceMainElement = function(elem) {
};

HatoholBootstrapDialog.prototype.appendToMainElement = function(elem) {
};

/**
 * Close and delete the dialog.
 */
HatoholBootstrapDialog.prototype.closeDialog = function() {
};

HatoholBootstrapDialog.prototype.setButtonState = function(buttonLabel, state) {
  var btn = $("#saveIncidentTrackerButton");
  if (state) {
     btn.removeAttr("disabled");
     btn.removeClass("ui-state-disabled");
  } else {
     btn.attr("disabled", "disable");
     btn.addClass("ui-state-disabled");
  }
};
