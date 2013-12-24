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

// ---------------------------------------------------------------------------
// HatoholItemRemover
// ---------------------------------------------------------------------------
var HatoholItemRemover = function(deleteParameters) {
  //
  // deleteParameters has following parameters.
  //
  // * id: Please enumerate an array of ID to delete.
  // * type
  // TODO: Add the description.
  //
  var count = 0;
  var total = 0;
  var errors = 0;
  var msg = "";
  for (var i = 0; i < deleteParameters.id.length; i++) {
    count++;
    total++;
    new HatoholConnector({
      url: '/' + deleteParameters.type + '/' + deleteParameters.id[i],
        request: "DELETE",
        context: deleteParameters.id[i],
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
          compleOneDel();
        }
    });
  }

  function compleOneDel() {
    count--;
    var completed = total - count;
    hatoholErrorMsgBox(msg + gettext("Deleting...") + " " + completed +
                       " / " + total);
    if (count > 0)
      return;

    // close dialogs
    hatoholInfoMsgBox(msg + gettext("Completed. (Number of errors: ") +
                      errors + "/" + total + ")");

    // update the main view
    startConnection(deleteParameters.type, updateCore);
  }
}
