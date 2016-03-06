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

describe('SeverityRanksView', function() {
  var TEST_FIXTURE_ID = 'severityRanksViewFixture';
  var severityRanksViewHTML;
  var defaultSeverityRanks = [
    {
      "id": 1,
      "status": 0,
      "color": "#BCBCBC",
      "label": "Not classified",
      "asImportant": false
    },
    {
      "id": 2,
      "status": 1,
      "color": "#CCE2CC",
      "label": "Information",
      "asImportant": false
    },
    {
      "id": 3,
      "status": 2,
      "color": "#FDFD96",
      "label": "Warning",
      "asImportant": false
    },
    {
      "id": 4,
      "status": 3,
      "color": "#DDAAAA",
      "label": "Error",
      "asImportant": true
    },
    {
      "id": 5,
      "status": 4,
      "color": "#FF8888",
      "label": "Critical",
      "asImportant": true
    },
    {
      "id": 6,
      "status": 5,
      "color": "#FF0000",
      "label": "Emergency",
      "asImportant": true
    }
  ];

  function getSeverityRanksJson(severityRanks) {
    return JSON.stringify({
      apiVersion: hatohol.FACE_REST_API_VERSION,
      errorCode: hatohol.HTERR_OK,
      SeverityRanks: severityRanks ? severityRanks : defaultSeverityRanks,
    });
  }

  function fakeAjax() {
    var requests = this.requests = [];
    this.xhr = sinon.useFakeXMLHttpRequest();
    this.xhr.onCreate = function(xhr) {
      requests.push(xhr);
    };
  }

  function restoreAjax() {
    this.xhr.restore();
  }

  function respond(severityRanksJson) {
    var request = this.requests[0];
    request.respond(200, { "Content-Type": "application/json" },
                    severityRanksJson ? severityRanksJson : getSeverityRanksJson());
  }

  function loadFixture(pathFromTop, onLoad) {
    var iframe = $("<iframe>", {
      id: "loaderFrame",
      src: "../../" + pathFromTop + "?start=false",
      load: function() {
        var html = $("#main", this.contentDocument).html();
        onLoad(html);
        $('#loaderFrame').remove();
      }
    });
    $('body').append(iframe);
  }

  beforeEach(function(done) {
    $('body').append($('<div>', { id: TEST_FIXTURE_ID }));
    var setupFixture = function(html) {
      severityRanksViewHTML = html;
      $("#" + TEST_FIXTURE_ID).append($("<div>", { id: "main" }));
      $("#main").html(severityRanksViewHTML);
      fakeAjax();
      done();
    };

    if (severityRanksViewHTML)
      setupFixture(severityRanksViewHTML);
    else
      loadFixture("ajax_severity_ranks", setupFixture);
  });

  afterEach(function() {
    restoreAjax();
    $("#" + TEST_FIXTURE_ID).remove();
    $(".sp-container").remove();
  });

  it('Base elements', function() {
    var operator = {
      "userId": 1,
      "name": "admin",
      "flags": hatohol.ALL_PRIVILEGES
    };
    var userProfile = new HatoholUserProfile(operator);
    var view = new SeverityRanksView(userProfile);
    respond();
    var heads = $('div#' + TEST_FIXTURE_ID + ' h2');

    expect(heads).to.have.length(1);
    expect($('#table')).to.have.length(1);
    expect($('tr')).to.have.length(defaultSeverityRanks.length + 1);
  });

  it('with update privilege', function() {
    var operator = {
      "userId": 1,
      "name": "admin",
      "flags": 1 << hatohol.OPPRVLG_UPDATE_SEVERITY_RANK
    };

    var userProfile = new HatoholUserProfile(operator);
    var view = new SeverityRanksView(userProfile);
    respond();

    var saveButton = $("#save-severity-ranks");
    expect(saveButton).to.have.length(1);
    expect(saveButton.is(":visible")).to.be(true);
  });

  it('with no update privilege', function() {
    var operator = {
      "userId": 2,
      "name": "guest",
      "flags": 0
    };
    var userProfile = new HatoholUserProfile(operator);
    var view = new SeverityRanksView(userProfile);
    respond();

    var saveButton = $("#save-severity-ranks");
    expect(saveButton).to.have.length(1);
    expect(saveButton.is(":visible")).to.be(false);
  });
});
