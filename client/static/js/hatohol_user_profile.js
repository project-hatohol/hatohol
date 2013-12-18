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

var HatoholUserProfile = function() {
  var self = this;

  this.user = null;
  this.onLoadCb = [];
  this.connector = new HatoholConnector({
    url: '/user/me',
    data: {},
    replyCallback: function(reply, parser) {
      var user = reply.users[0];
      var i;

      setUserProfile(user);
      for (i = 0; i < self.onLoadCb.length; ++i)
        self.onLoadCb[i](user);
    },
    parseErrorCallback: hatoholErrorMsgBoxForParser,
    connectErrorCallback: function(XMLHttpRequest, textStatus, errorThrown) {
    }
  });

  function setUserProfile(user) {
    self.user = user;
    $("#currentUserName").text(self.user.name);

    $("#logoutMenuItem").click(function() {
      new HatoholConnector({
        url: '/logout',
        replyCallback: function(reply, parser) {
          document.location.href = "ajax_dashboard";
        },
        parseErrorCallback: hatoholErrorMsgBoxForParser
      });
    });

    $("#changePasswordMenuItem").click(function() {
      new HatoholPasswordChanger();
    });
  }
};

HatoholUserProfile.prototype.addOnLoadCb = function(onLoadCb) {
  if (userProfile.user) {
    onLoadCb();
  } else {
    userProfile.onLoadCb.push(onLoadCb);
  };
};

HatoholUserProfile.prototype.hasFlags = function(flags) {
  if (!this.user)
    return false;
  return this.user.flags & flags;
};
