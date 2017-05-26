/*
 * Copyright (C) 2013-2014 Project Hatohol
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

var UsersView = function(userProfile) {
  //
  // Variables
  //
  var self = this;

  // call the constructor of the super class
  HatoholMonitoringView.apply(this, [userProfile]);

  //
  // main code
  //
  if (userProfile.hasFlag(hatohol.OPPRVLG_CREATE_USER))
    $("#add-user-button").show();
  if (userProfile.hasFlag(hatohol.OPPRVLG_DELETE_USER))
    $("#delete-user-button").show();
  self.startConnection('user', updateCore);

  //
  // Main view
  //
  $("#add-user-button").click(function() {
    new HatoholUserEditDialog({
      operatorProfile: userProfile,
      succeededCallback: addOrEditSucceededCb
    });
  });

  $("#delete-user-button").click(function() {
    var msg = gettext("Delete the selected items ?");
    hatoholNoYesMsgBox(msg, deleteUsers);
  });

  $("#edit-user-roles-button").click(function() {
    new HatoholUserRolesEditor({
      operatorProfile: userProfile,
      changedCallback: function() {
	self.startConnection('user', updateCore);
      },
    });
  });

  function addOrEditSucceededCb() {
    self.startConnection('user', updateCore);
  }

  //
  // Event handlers for forms in the main view
  //
  function setupEditLinksAndButtons(reply)
  {
    var i, id, users = reply["users"], usersMap = {};
    for (i = 0; i < users.length; ++i) {
      usersMap[users[i]["userId"]] = users[i];
    }

    for (i = 0; i < users.length; ++i) {
      id = "#edit-user" + users[i]["userId"];
      $(id).click(function() {
        var userId = this.getAttribute("userId");
        new HatoholUserEditDialog({
          operatorProfile: userProfile,
          succeededCallback: addOrEditSucceededCb,
          targetUser: usersMap[userId]
        });
      });

      id = id + "-privileges";
      $(id).click(function() {
        var userId = this.getAttribute("userId");
        new HatoholPrivilegeEditDialog(userId, applyPrivilegesCb);
      });
    }

    if (userProfile.hasFlag(hatohol.OPPRVLG_UPDATE_USER)) {
      $(".privilege-column").show();
    }
  }

  function applyPrivilegesCb() {
    self.startConnection('user', updateCore);
  }

  //
  // Commonly used functions from a dialog.
  //

  //
  // delete-user dialog
  //
  function deleteUsers() {
    var delId = [], userId;
    var checkbox = $(".selectcheckbox");
    $(this).dialog("close");
    for (var i = 0; i < checkbox.length; i++) {
      if (!checkbox[i].checked)
        continue;
      userId = checkbox[i].getAttribute("userId");
      delId.push(userId);
    }
    new HatoholItemRemover({
      id: delId,
      type: "user",
      completionCallback: function() {
        self.startConnection("user", updateCore);
      },
    });
    hatoholInfoMsgBox(gettext("Deleting..."));
  }

  function getUserTypeFromFlags(flags, reply) {
    rolesMap = reply.userRoles;
    switch(flags) {
    case hatohol.ALL_PRIVILEGES:
      return gettext("Admin");
    case hatohol.NONE_PRIVILEGE:
      return gettext("Guest");
    default:
      if (rolesMap && rolesMap[flags])
        return escapeHTML(rolesMap[flags].name);
      break;
    }
    return gettext("Unknown");
  }

  //
  // callback function from the base template
  //
  function drawTableBody(reply)
  {
    let html = "";

    for (let user of reply["users"])
    {
      html += "<tr>";
      html += "<td class='delete-selector' style='display:none;'>";
      html += "<input type='checkbox' class='selectcheckbox'";
      html += "value='" + reply["users"].indexOf(user) + "'";
      html += "userId='" + escapeHTML(user["userId"]) + "'>";
      html += "</td>";
      html += "<td>" + escapeHTML(user["userId"]) + "</td>";
      html += "<td>";
      if (userProfile.hasFlag(hatohol.OPPRVLG_UPDATE_USER))
      {
        html += "<a href='javascript:void(0);' id='edit-user" +
          escapeHTML(user["userId"]);
        html += "' userId='"+ escapeHTML(user["userId"]) + "'>" +
          escapeHTML(user["name"])   + "</a></td>";
      }
      else
      {
        html += escapeHTML(user["name"]);
      }
      html += "<td>" + getUserTypeFromFlags(user["flags"], reply)  + "</td>";
      if (user["flags"] == hatohol.ALL_PRIVILEGES)
      {
        html += "<td class='privilege-column' style='display:none;'>";
        html += gettext("All") + "</td>";
      }
      else
      {
        html += "<td class='privilege-column' style='display:none;'>";
        html += "<form class='form-inline' style='margin: 0'>";
        html += "  <input id='edit-user" + escapeHTML(user["userId"]) + "-privileges'";
        html += "    type='button' class='btn btn-default'";
        html += "    userId='" + escapeHTML(user["userId"]) + "'";
        html += "    value='" + gettext("Show / Edit") +  "' />";
        html += "</form>";
        html += "</td>";
      }
      html += "</tr>";
    }

    return html;
  }

  function updateCore(reply) {
    $("#table tbody").empty();
    $("#table tbody").append(drawTableBody(reply));
    self.setupCheckboxForDelete($("#delete-user-button"));
    if (self.userProfile.hasFlag(hatohol.OPPRVLG_DELETE_USER))
      $(".delete-selector").shiftcheckbox();
      $(".delete-selector").show();
    setupEditLinksAndButtons(reply);
    self.displayUpdateTime();
  }
};

UsersView.prototype = Object.create(HatoholMonitoringView.prototype);
UsersView.prototype.constructor = UsersView;
