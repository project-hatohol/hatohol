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

describe('CustomIncidentLabelsView', function() {
  var TEST_FIXTURE_ID = 'customIncidentLabelsFixture';
  var customIncidentLabelsViewHTML;
  var defaultCustomIncidentLabels = [
    {
      "id": 1,
      "code": "NONE",
      "label": ""
    },
    {
      "id": 2,
      "code": "IN PROGRESS",
      "label": ""
    },
    {
      "id": 3,
      "code": "HOLD",
      "label": ""
    },
    {
      "id": 4,
      "code": "DONE",
      "label": ""
    },
    {
      "id": 5,
      "code": "USER01",
      "label": ""
    },
    {
      "id": 6,
      "code": "USER02",
      "label": ""
    },
    {
      "id": 7,
      "code": "USER03",
      "label": ""
    },
    {
      "id": 8,
      "code": "USER04",
      "label": ""
    },
    {
      "id": 9,
      "code": "USER05",
      "label": ""
    },
    {
      "id": 10,
      "code": "USER06",
      "label": ""
    }
  ];

  function getCustomIncidentLabelsJson(customIncidentLabels) {
    return JSON.stringify({
      apiVersion: hatohol.FACE_REST_API_VERSION,
      errorCode: hatohol.HTERR_OK,
      CustomIncidentStatuses: customIncidentLabels ? customIncidentLabels : defaultCustomIncidentLabels,
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

  function respond(customIncidentLabelsJson) {
    var request = this.requests[0];
    request.respond(200, { "Content-Type": "application/json" },
                    customIncidentLabelsJson ? customIncidentLabelsJson : getCustomIncidentLabelsJson());
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
      customIncidentLabelsViewHTML = html;
      $("#" + TEST_FIXTURE_ID).append($("<div>", { id: "main" }));
      $("#main").html(customIncidentLabelsViewHTML);
      fakeAjax();
      done();
    };

    if (customIncidentLabelsViewHTML)
      setupFixture(customIncidentLabelsViewHTML);
    else
      loadFixture("ajax_custom_incident_labels", setupFixture);
  });

  afterEach(function() {
    restoreAjax();
    $("#" + TEST_FIXTURE_ID).remove();
  });

  it('Base elements', function() {
    var operator = {
      "userId": 1,
      "name": "admin",
      "flags": hatohol.ALL_PRIVILEGES
    };
    var userProfile = new HatoholUserProfile(operator);
    var view = new CustomIncidentLabelsView(userProfile);
    respond();
    var heads = $('div#' + TEST_FIXTURE_ID + ' h2');

    expect(heads).to.have.length(1);
    expect($('#table')).to.have.length(1);
    expect($('tr')).to.have.length(defaultCustomIncidentLabels.length + 1);
  });

  it('with update privilege', function() {
    var operator = {
      "userId": 1,
      "name": "admin",
      "flags": 1 << hatohol.OPPRVLG_UPDATE_INCIDENT_SETTING
    };

    var userProfile = new HatoholUserProfile(operator);
    var view = new CustomIncidentLabelsView(userProfile);
    respond();

    var saveButton = $("#save-custom-incident-labels");
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
    var view = new CustomIncidentLabelsView(userProfile);
    respond();

    var saveButton = $("#save-custom-incident-labels");
    expect(saveButton).to.have.length(1);
    expect(saveButton.is(":visible")).to.be(false);
  });
});
