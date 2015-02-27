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

// ---------------------------------------------------------------------------
// HatoholItemUpdate
// ---------------------------------------------------------------------------
var HatoholItemUpdate = function(updateParameters, connParameters) {
  //
  // updateParameters has following parameters.
  //
  // * id: An array of ID to update.
  // * type: A type to be included in the request URL.
  // * completionCallback: Callback to be executed after completion.
  //
  var count = 0;
  var total = 0;
  var errors = 0;
  var msg = "";
  for (var i = 0; i < updateParameters.id.length; i++) {
    count++;
    total++;
    new HatoholConnector($.extend({
      url: '/' + updateParameters.type + '/' + updateParameters.id[i],
        request: "PUT",
        context: updateParameters.id[i],
        replyCallback: function(reply, parser, context) {
        },
        parseErrorCallback: function(reply, parser) {
          // TODO: Add line break.
          msg += "errorCode: " + reply.errorCode + ". ";
          hatoholErrorMsgBox(msg);
          errors++;
        },
        connectErrorCallback: function(XMLHttpRequest,
                                textStatus, errorThrown) {
          // TODO: Add line break.
          msg += "Error: " + XMLHttpRequest.status + ": " +
          XMLHttpRequest.statusText + ". ";
          hatoholErrorMsgBox(msg);
          errors++;
        },
        completionCallback: function(context) {
          compleOneUpdate();
        }
    }, connParameters || {}));
  }

  function compleOneUpdate() {
    count--;
    var completed = total - count;
    hatoholErrorMsgBox(msg + gettext("Reloading...") + " " + completed +
                       " / " + total);
    if (count > 0)
      return;

    // close dialogs
    hatoholInfoMsgBox(msg + gettext("Completed. (Number of errors: ") +
                      errors + "/" + total + ")");

    if (updateParameters.completionCallback)
      updateParameters.completionCallback();
  }
}
