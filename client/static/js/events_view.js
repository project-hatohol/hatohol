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
    /* Not supported yet
    s += '      <th data-sort="int">' + gettext('Comment') + '</th>';
    s += '      <th data-sort="int">' + gettext('Action') + '</th>';
    */
    s += '    </tr>';
    s += '  </thead>';
    s += '  <tbody>';
    s += '  </tbody>';
    s += '</table>';

    s += '<center>';
    s += '<form class="form-inline">';
    s += '  <input id="latest-events-button" type="button" class="btn-info" value="' + gettext('Latest events') + '" />';
    s += '  <input id="next-events-button" type="button" class="btn-primary" value="' + gettext('To next') + '" />';
    s += '</form>';
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

    $('#latest-events-button').click(function() {
      connParam.url = '/event?maximumNumber=' + self.numEventsPerPage
                      + '&sortOrder=' + self.sortOrder;
      self.connector.start(connParam);
      $(self.baseElem).scrollTop(0);
    });
  }

  function parseData(replyData) {
    var parsedData = new Object();
    var server, triggerId;
    var x, event;
    var allTimes, serverNames;
    var times, durations, now;

    parsedData.durations = {};

    // extract server names & times from raw data
    allTimes = {};
    for (x = 0; x < replyData["events"].length; ++x) {
      event = replyData["events"][x];
      server = replyData["servers"][event["serverId"]]["name"];
      triggerId = event["triggerId"];

      if (!allTimes[server])
        allTimes[server] = {};
      if (!allTimes[server][triggerId])
        allTimes[server][triggerId] = [];
      
      allTimes[server][triggerId].push(event["time"]);
    }

    // create server names array & durations map
    serverNames = [];
    for (server in allTimes) {
      // store the unique server name
      serverNames.push(server);

      // calculate durations
      for (triggerId in allTimes[server]) {
        times = allTimes[server][triggerId].uniq().sort();
        durations = {};
        for (x = 0; x < times.length; ++x) {
          if (x == times.length - 1) {
            now = (new Date()).getTime() / 1000;
            durations[times[x]] = now - Number(times[x]);
          } else {
            durations[times[x]] = Number(times[x + 1]) - Number(times[x]);
          }
        }
        allTimes[server][triggerId] = durations;
      }
      parsedData.durations[server] = allTimes[server];
    }
    parsedData.serverNames = serverNames.sort();

    return parsedData;
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
    var klass, serverName, hostName, clock, status, severity, duration;
    var server, event, html = "";
    var x;

    for (x = 0; x < rd["events"].length; ++x) {
      event      = rd["events"][x];
      server     = rd["servers"][event["serverId"]];
      serverName = server["name"];
      hostName   = server["hosts"][event["hostId"]]["name"];
      clock      = event["time"];
      status     = event["type"];
      severity   = event["severity"];
      duration   = pd.durations[serverName][event["triggerId"]][clock];
      klass      = serverName;
      html += "<tr class='" + klass + "'>";
      html += "<td>" + serverName + "</td>";
      html += "<td data-sort-value='" + clock + "'>" + formatDate(clock) + "</td>";
      html += "<td>" + hostName + "</td>";
      html += "<td>" + event["brief"] + "</td>";
      html += "<td class='status" + status + "' data-sort-value='" + status + "'>" + status_choices[Number(status)] + "</td>";
      html += "<td class='severity" + severity + "' data-sort-value='" + severity + "'>" + severity_choices[Number(severity)] + "</td>";
      html += "<td data-sort-value='" + duration + "'>" + formatSecond(duration) + "</td>";
      /*
      html += "<td>" + "unsupported" + "</td>";
      html += "<td>" + "unsupported" + "</td>";
      */
      html += "</tr>";
      fixupUnifiedIdInfo(event);
    }

    return html;
  }

  function updateCore(reply) {
    var rawData = reply;
    var parsedData = parseData(rawData);

    target = '#select-server';
    $(target).children('option').remove();
    $(target).append('<option>---------</option>');
    setCandidate($(target), parsedData.serverNames);

    $("#table tbody").empty();
    $("#table tbody").append(drawTableBody(rawData, parsedData));
  }
};
