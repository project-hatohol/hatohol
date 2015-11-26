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

var HatoholNavi = function(userProfile, currentPage) {
  var self = this;
  var i, j, title, klass;
  var item, children, child, dropDown;
  var menuItems = [
    {
      title: gettext("Dashboard"),
      href:  "ajax_dashboard"
    },
    {
      title: gettext("Overview : Triggers"),
      href:  "ajax_overview_triggers"
    },
    {
      title: gettext("Overview : Items"),
      href:  "ajax_overview_items"
    },
    {
      title: gettext("Latest data"),
      href:  "ajax_latest"
    },
    {
      title: gettext("Triggers"),
      href:  "ajax_triggers" },
    {
      title: gettext("Events"),
      href:  "ajax_events"
    },
    {
      title: gettext("Settings"),
      children: [
        {
          title: gettext("Monitoring Servers"),
          href:  "ajax_servers"
        },
        {
          title: gettext("Actions"),
          href:  "ajax_actions"
        },
        {
          title: gettext("Graphs"),
          href:  "ajax_graphs"
        },
        {
          title: gettext("Incident tracking"),
          href:  "ajax_incident_settings",
          flags: [
            hatohol.OPPRVLG_CREATE_INCIDENT_SETTING,
            hatohol.OPPRVLG_UPDATE_INCIDENT_SETTING,
            hatohol.OPPRVLG_DELETE_INCIDENT_SETTING,
            hatohol.OPPRVLG_GET_ALL_INCIDENT_SETTINGS,
          ]
        },
        {
          title: gettext("Log search systems"),
          href:  "ajax_log_search_systems"
        },
        {
          title: gettext("Users"),
          href:  "ajax_users",
          flags: [
            hatohol.OPPRVLG_CREATE_USER,
            hatohol.OPPRVLG_UPDATE_USER,
            hatohol.OPPRVLG_DELETE_USER,
            hatohol.OPPRVLG_GET_ALL_USERS,
          ]
        },
        {
          title: gettext("Severity Ranks"),
          href:  "ajax_severity_ranks",
          flags: [
            hatohol.OPPRVLG_CREATE_SEVERITY_RANK,
            hatohol.OPPRVLG_UPDATE_SEVERITY_RANK,
            hatohol.OPPRVLG_DELETE_SEVERITY_RANK,
          ]
        },
        {
          title: gettext("Custom Incident Labels"),
          href:  "ajax_custom_incident_labels",
          flags: [
            hatohol.OPPRVLG_CREATE_INCIDENT_SETTING,
            hatohol.OPPRVLG_UPDATE_INCIDENT_SETTING,
            hatohol.OPPRVLG_DELETE_INCIDENT_SETTING,
          ]
        },
      ]
    },
    {
      title: gettext("Help"),
      children: [
        {
          title: gettext("Online Documents"),
          href: "http://www.hatohol.org/docs"
        },
        {
          title: gettext("Hatohol version: ") + HATOHOL_VERSION,
          href: "#version"
        },
      ]
    },
  ];

  var getCurrentPage = function() {
    var matchResults;

    if (currentPage) {
      return currentPage;
    } else if (location.pathname.match(".*/$")) {
      return menuItems[0].href;
    } else {
      matchResults = location.pathname.match(".*/(.+)$");
      if (matchResults && matchResults.length > 1)
        return matchResults[1];
      else
        return location.pathname;
    }
    return undefined;
  };

  var pageIsEnabled = function(menuItem) {
    var i;

    if (!hatohol.enabledPages)
      return true;
    if (Object.keys(hatohol.enabledPages).length <= 0)
      return true;

    if (menuItem.children) {
        for (i = 0; i < menuItem.children.length; i++) {
          if (pageIsEnabled(menuItem.children[i]))
            return true;
        }
        return false;
    } else {
        return !!hatohol.enabledPages[menuItem.href];
    }
  };

  var createMenuItem = function(menuItem) {
    if (menuItem.flags != undefined &&
        !userProfile.hasFlags(menuItem.flags))
    {
      return null;
    }

    var href = "";
    if (HATOHOL_PROJECT_TOP_PATH.length > 0)
        href = HATOHOL_PROJECT_TOP_PATH + "/";
    href = href + menuItem.href;

    if (menuItem.children) {
      title = '<a href="#" class="dropdown-toggle" data-toggle="dropdown">' +
        menuItem.title + '<span class="caret"></span>' + '</a>';
      klass = 'dropdown';
    } else if (href == self.currentPage) {
      title = '<a>' + menuItem.title + '</a>';
      klass = "active";
    } else {
      title = '<a href=' + href + '>' +
	menuItem.title + '</a>';
      klass = undefined;
    }

    return $("<li/>", {
      html: title,
      class: klass,
    });
  };

  self.currentPage = getCurrentPage();

  for (i = 0; i < menuItems.length; ++i) {
    if (!pageIsEnabled(menuItems[i]))
      continue;

    item = createMenuItem(menuItems[i]);
    if (!item)
      continue;

    if (menuItems[i].children) {
      dropDown = $("<ul>", {
        class: "dropdown-menu",
      });
      item.append(dropDown);

      children = menuItems[i].children;
      for (j = 0; j < children.length; j++) {
        if (!pageIsEnabled(children[j]))
          continue;
        child = createMenuItem(children[j]);
        if (child)
          dropDown.append(child);
      }
    }
    item.appendTo("ul.nav:first");
  };
};
