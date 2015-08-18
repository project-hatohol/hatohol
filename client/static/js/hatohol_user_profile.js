/*
 * Copyright (C) 2013 Project Hatohol
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

var HatoholUserProfile = function(user) {
  var self = this;

  this.user = null;
  this.onLoadCb = [];
  this.connector = null;
  if (user)
    this.user = user;
  else
    this.connector = load();

  function load() {
    return new HatoholConnector({
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
  }

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
      new HatoholPasswordChanger(user);
    });
  };
};

HatoholUserProfile.prototype.addOnLoadCb = function(onLoadCb) {
  if (this.user) {
    onLoadCb(this.user);
  } else {
    this.onLoadCb.push(onLoadCb);
  };
};

HatoholUserProfile.prototype.hasFlag = function(flag, user) {
  if (!user)
    user = this.user;
  return this.hasFlags((1 << flag), user);
};

HatoholUserProfile.prototype.hasFlags = function(flags, user) {
  if (!user)
    user = this.user;
  if (!user)
    return false;
  return this.user.flags & flags;
};
