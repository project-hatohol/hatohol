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

var EventsView = function(baseElem) {

  var self = this;
  self.baseElem = baseElem;
  self.minUnifiedId = null;
  self.maxUnifiedId = null;

  var status_choices = [gettext('OK'), gettext('Problem'), gettext('Unknown')];
  var severity_choices = [
    gettext('Not classified'), gettext('Information'), gettext('Warning'),
    gettext('Average'), gettext('High'), gettext('Disaster')];

  var connParam =  {
    replyCallback: function(reply, parser) {
      // TODO: don't use global function updateScreen().
      updateScreen(reply, updateCore);
    },
    parseErrorCallback: function(reply, parser) {
      hatoholErrorMsgBoxForParser(reply, parser);

      // TODO: don't use global function updateScreen().
      setStatus({
        "class" : "danger",
        "label" : gettext("ERROR"),
        "lines" : [ msg ],
      });
    }
  };

  self.userConfig = new HatoholUserConfig(); 
  start();

  //
  // Private functions 
  //
  function start() {
    var DEFAULT_NUM_EVENTS_PER_PAGE = 50;
    var DEFAULT_SORT_ORDER = hatohol.DATA_QUERY_OPTION_SORT_DESCENDING;
    self.userConfig.get({
      itemNames:['num-events-per-page', 'event-sort-order'],
      successCallback: function(conf) {
        self.numEventsPerPage =
          self.userConfig.findOrDefault(conf, 'num-events-per-page',
                                        DEFAULT_NUM_EVENTS_PER_PAGE);
        self.sortOrder = 
          self.userConfig.findOrDefault(conf, 'event-sort-order',
                                        DEFAULT_SORT_ORDER);
        createPage();
      },
      connectErrorCallback: function(XMLHttpRequest, textStatus, errorThrown) {
        // TODO: implement
      },
    });
  }

  function createPage() {
    createUI(self.baseElem);
    setupEvents();
    connParam.url = '/event?maximumNumber=' + self.numEventsPerPage + '&sortOrder=' + self.sortOrder;
    self.connector = new HatoholConnector(connParam);
  }

  function createUI(elem) {
    var s = '';
    s += '<h2>' + gettext('Events') + '</h2>';

    s += '<form class="form-inline">';
    s += '  <label>' + gettext('Server:') + '</label>';
    s += '  <select id="select-server">';
    s += '    <option>---------</option>';
    s += '  </select>';
    s += '  <label for="num-events-per-page">' + gettext("# of events per page") + '</label>'
    s += '  <input type="text" class="input-mini" id="num-events-per-page">';
    s += '</form>';

    s += '<table class="table table-condensed table-hover" id="table">';
    s += '  <thead>';
    s += '    <tr>';
    s += '      <th data-sort="string">' + gettext('Server') + '</th>';
    s += '      <th data-sort="int">' + gettext('Time') + '</th>';
    s += '      <th data-sort="string">' + gettext('Host') + '</th>';
    s += '      <th data-sort="string">' + gettext('Brief') + '</th>';
    s += '      <th data-sort="int">' + gettext('Status') + '</th>';
    s += '      <th data-sort="int">' + gettext('Severity') + '</th>';
    s += '      <th data-sort="int">' + gettext('Duration') + '</th>';
    s += '      <th data-sort="int">' + gettext('Comment') + '</th>';
    s += '      <th data-sort="int">' + gettext('Action') + '</th>';
    s += '    </tr>';
    s += '  </thead>';
    s += '  <tbody>';
    s += '  </tbody>';
    s += '</table>';

    s += '<center>';
    s += '<button id="next-events-button" class="btn-primary">' + gettext('To next') + '</button>';
    s += '</center>';
    s += '<br>';

    $(elem).append(s);
  }

  function setupEvents() {
    $("#table").stupidtable();
    $("#table").bind('aftertablesort', function(event, data) {
      var th = $(this).find("th");
      th.find("i.sort").remove();
      var icon = data.direction === "asc" ? "up" : "down";
      th.eq(data.column).append("<i class='sort icon-arrow-" + icon +"'></i>");
    });

    $("#select-server").change(function() {
      chooseRow();
    });

    $('#num-events-per-page').val(self.numEventsPerPage);
    $('#num-events-per-page').change(function() {
      var val = parseInt($('#num-events-per-page').val());
      if (!isFinite(val))
        val = self.numEventsPerPage;
      $('#num-events-per-page').val(val);
      self.numEventsPerPage = val;

      var params = {
        items: {'num-events-per-page': val},
        successCallback: function(){ /* we just ignore it */ },
        connectErrorCallback: function() {
          // TODO: show an error message
        },
      };
      self.userConfig.store(params);
    });

    $('#next-events-button').click(function() {
      connParam.url = '/event?maximumNumber=' + self.numEventsPerPage
                      + '&sortOrder=' + self.sortOrder
                      + '&startId=' + (self.minUnifiedId - 1);
      self.connector.start(connParam);
      $(self.baseElem).scrollTop(0);
    });
  }

  function parseData(rd) {
    var pd = new Object();
    var o, server, triggerId;
    var x;
    var ts, ss;
    var list, map;

    pd.diffs = new Object();

    ts = new Object();
    for (x = 0; x < rd["events"].length; ++x) {
      o = rd["events"][x];
      server    = rd["servers"][o["serverId"]]["name"];
      triggerId = o["triggerId"];
      if ( !(server in ts) ) {
        ts[server] = new Object();
      }
      if ( !(triggerId in ts[server]) ) {
        ts[server][triggerId] = [];
      }
      ts[server][triggerId].push(o["time"]);
    }

    ss = [];
    for ( server in ts ) {
      ss.push(server);
      for ( triggerId in ts[server] ) {
        list = ts[server][triggerId].uniq().sort();
        map = new Object();
        for (x = 0 ; x < list.length; ++x) {
          if ( x + 1 < list.length ) {
            map[list[x]] = Number(list[x + 1]) - Number(list[x]);
          }
          else {
            map[list[x]] = (new Date()).getTime() / 1000 - Number(list[x]);
          }
        }
        ts[server][triggerId] = map;
      }
      pd.diffs[server] = ts[server];
    }
    pd.servers = ss.sort();

    return pd;
  }

  function fixupUnifiedIdInfo(eventObj) {
    var unifiedId = eventObj.unifiedId;
    if (!self.minUnifiedId)
      self.minUnifiedId = unifiedId;
    else if (unifiedId < self.minUnifiedId)
      self.minUnifiedId = unifiedId;

    if (!self.maxUnifiedId)
      self.maxUnifiedId = unifiedId;
    else if (unifiedId > self.maxUnifiedId)
      self.maxUnifiedId = unifiedId;
  }

  function drawTableBody(rd, pd) {
    var s = "";
    var o;
    var x;
    var klass, server, host, clock, status, severity, diff;

    for (x = 0; x < rd["events"].length; ++x) {
      o = rd["events"][x];
      server   = rd["servers"][o["serverId"]]["name"];
      host     = rd["servers"][o["serverId"]]["hosts"][o["hostId"]]["name"];
      clock    = o["time"];
      status   = o["type"];
      severity = o["severity"];
      diff     = pd.diffs[server][o["triggerId"]][clock];
      klass    = server;
      s += "<tr class='" + klass + "'>";
      s += "<td>" + server + "</td>";
      s += "<td data-sort-value='" + clock + "'>" + formatDate(clock) + "</td>";
      s += "<td>" + host + "</td>";
      s += "<td>" + o["brief"] + "</td>";
      s += "<td class='status" + status + "' data-sort-value='" + status + "'>" + status_choices[Number(status)] + "</td>";
      s += "<td class='severity" + severity + "' data-sort-value='" + severity + "'>" + severity_choices[Number(severity)] + "</td>";
      s += "<td data-sort-value='" + diff + "'>" + formatSecond(diff) + "</td>";
      s += "<td>" + "unsupported" + "</td>";
      s += "<td>" + "unsupported" + "</td>";
      s += "</tr>";
      fixupUnifiedIdInfo(o);
    }

    return s;
  }

  function updateCore(reply) {
    var rawData = reply;
    var parsedData = parseData(rawData);

    setCandidate($("#select-server"), parsedData.servers);

    $("#table tbody").empty();
    $("#table tbody").append(drawTableBody(rawData, parsedData));
  }
};
