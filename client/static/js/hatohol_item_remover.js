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
  for (var i = 0; i < deleteParameters.id.length; i++) {
    count++;
    total++;
    new HatoholConnector({
      url: '/' + deleteParameters.type + '/' + deleteParameters.id[i],
        request: "DELETE",
        context: deleteParameters.id[i],
        replyCallback: function(deletedItem, parser, context) {
          parseDeleteResult(deletedItem);
        },
        connectErrorCallback: function(XMLHttpRequest,
                                textStatus, errorThrown) {
          var errorMsg = "Error: " + XMLHttpRequest.status + ": " +
          XMLHttpRequest.statusText;
          hatoholErrorMsgBox(errorMsg);
          errors++;
        },
        completionCallback: function(context) {
          compleOneDel();
        }
    });
  }

  function checkParseResult(deletedItem) {
    var msg;
    var malformed =false;
    if (deletedItem.result == undefined)
      malformed = true;
    if (!malformed && !deletedItem.result && deletedItem.message == undefined)
      malformed = true;
    if (malformed) {
      msg = "The returned content is malformed: " +
        "Not found 'result' or 'message'.\n" +
        JSON.stringify(deletedItem);
      hatoholErrorMsgBox(msg);
      return false;
    }
    if (!deletedItem.result) {
      msg = "Failed:\n" + deletedItem.message;
      hatoholErrorMsgBox(msg);
      return false;
    }
    if (deletedItem.id == undefined) {
      msg = "The returned content is malformed: " +
        "'result' is true, however, 'id' missing.\n" +
        JSON.stringfy(deletedItem);
      hatoholErrorMsgBox(msg);
      return false;
    }
    return true;
  }

  function parseDeleteResult(deletedItem) {
    if (!checkParseResult(deletedItem))
      return;
    if (!(deletedItem.id in id)) {
      alert("Fatal Error: You should reload page.\nID: " + deletedItem.id +
          " is not in idArray: " + id);
      errors++;
      return;
    }
    delete id[deletedItem.id];
  }

  function compleOneDel() {
    count--;
    var completed = total - count;
    hatoholErrorMsgBox(gettext("Deleting...") + " " + completed +
                       " / " + total);
    if (count > 0)
      return;

    // close dialogs
    hatoholInfoMsgBox(gettext("Completed. (Number of errors: ") +
                      errors + ")");

    // update the main view
    startConnection(deleteParameters.type, updateCore);
  }
}
