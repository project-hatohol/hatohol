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

describe("GraphsView", function() {
  var TEST_FIXTURE_ID = "graphsViewFixture";
  var graphsViewHTML;
  var defaultGraphs = [
    {
      "id": 2,
      "user_id": 1,
      title: "Graph1",
      histories: [
        {
          "serverId": 1,
          "hostId": "10101",
          "itemId": "10002"
        },
        {
          "serverId": 2,
          "hostId": "10102",
          "itemId": "10003"
        }
      ]
    },
  ];

  function getOperator() {
    var operator = {
      "userId": 1,
      "name": "admin",
      "flags": hatohol.ALL_PRIVILEGES
    };
    return new HatoholUserProfile(operator);
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

  function loadFixture(pathFromTop, onLoad) {
    var iframe = $("<iframe>", {
      id: "loaderFrame",
      src: "../../" + pathFromTop + "?start=false",
      load: function() {
        var html = $("#main", this.contentDocument).html();
        onLoad(html);
        $("#loaderFrame").remove()
      }
    })
    $("body").append(iframe);
  }

  function respond(graphs) {
    var header = { "Content-Type": "application/json" };
    this.requests[0].respond(200, header, JSON.stringify(graphs));
  }

  beforeEach(function(done) {
    $("body").append($("<div>", { id: TEST_FIXTURE_ID }));
    var setupFixture = function(html) {
      graphsViewHTML = html;
      $("#" + TEST_FIXTURE_ID).append($("<div>", { id: "main" }))
      $("#main").html(graphsViewHTML);
      fakeAjax();
      done();
    };

    if (graphsViewHTML)
      setupFixture(graphsViewHTML);
    else
      loadFixture("ajax_graphs", setupFixture)
  });

  afterEach(function() {
    restoreAjax();
    $("#" + TEST_FIXTURE_ID).remove();
  });

  it("Base elements", function() {
    var view = new GraphsView(getOperator());
    var heads = $("div#" + TEST_FIXTURE_ID + " h2");

    expect(heads).to.have.length(1);
    expect($("#table")).to.have.length(1);
    expect($("tr")).to.have.length(1);
  });

  it('load a graph setting', function() {
    var view = new GraphsView(getOperator());
    var expected =
      '<td class="delete-selector" style="">' +
      '<input class="selectcheckbox" graphid="2" type="checkbox">' +
      '</td>' +
      '<td>2</td>' +
      '<td><a href="ajax_history?id=2">Graph1</a></td>'

    respond(defaultGraphs);
    expect(view).to.be.a(GraphsView);
    expect(requests[0].url).to.be(
      "/graphs/");
    expect($('tr')).to.have.length(defaultGraphs.length + 1);
    expect($('tr').eq(1).html()).to.contain(expected);
  });
});
