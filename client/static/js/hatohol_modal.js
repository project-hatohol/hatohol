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

// This class is supposed to replace HatoholDialog. However, the current
// implementation has not completed to do it.

//
// @param params
// An object that shall contain the following properties.
//
// - id [mandatory]
// An ID of the modal
//
// - title [optional]
// A title string of the modal. If this property is null or undefined,
// the modal header is not created.
//
// - body [mandatory]
// The body of the modal.
//
// - footer [optional]
// A footer elements. If this is null or undefined, the modal footer is
// not created.
//
var HatoholModal = function(params) {
  var self = this;
  self.owner = false;

  self.modalId = params.id;
  if ($("#" + self.modalId).length) {
    // When the modal has already been created
    return;
  }

  self.owner = true;
  self.modal = $('<div class="modal fade" id="' + params.id + '" tabindex="-1" role="dialog" />');
  $("body").append(self.modal);

  var dialog = $('<div class="modal-dialog" role="document" />');
  self.modal.append(dialog);

  var content = $('<div class="modal-content" />');
  dialog.append(content);

  if (params.title != null) {
    var header = $('<div class="modal-header" />');
    content.append(header);
    var title = $('<h4 class="modal-title">' + params.title + '</h4>');
    header.append(title);
  }

  var body = $('<div class="modal-body" />');
  content.append(body);
  body.append(params.body);
  self.body = body;

  if (params.footer != null) {
    var footer = $('<div class="modal-footer" />');
    content.append(footer);
    footer.append(params.footer);
  }
};

HatoholModal.prototype.show = function() {
  $("#" + this.modalId).modal({
    backdrop: "static",
    keyboard: false,
  });
};

HatoholModal.prototype.close = function(doneHandler) {
  var self = this;
  var modal = $("#" + this.modalId);
  modal.modal("hide");
  modal.off("hidden.bs.modal");
  modal.on("hidden.bs.modal", function(e) {
    modal.remove();
    if (doneHandler != undefined)
      doneHandler();
  });
};

HatoholModal.prototype.updateBody = function(newBody) {
  this.body.empty();
  this.body.append(newBody);
};

HatoholModal.prototype.isOwner = function() {
  return this.owner;
};
