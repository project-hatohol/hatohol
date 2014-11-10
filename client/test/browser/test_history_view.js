describe('HistoryView', function() {
  var TEST_FIXTURE_ID = 'historyViewFixture';
  var viewHTML;
  var defaultItems = [
    {
      "id": 1,
      "serverId": 1,
      "hostId": "10101",
      "brief": "cpu usage",
      "lastValueTime": 1415232279,
      "lastValue": "54.282349",
      "prevValue": "24.594534",
      "itemGroupName": "group1",
      "unit": "%",
      "valueType": hatohol.ITEM_INFO_VALUE_TYPE_FLOAT,
    },
  ];
  var defaultServers = {
    "1": {
      "name": "Zabbix",
      "type": 0,
      "ipAddress": "192.168.1.100",
      "hosts": {
        "10101": {
          "name": "Host1",
        },
      },
    },
  };

  function itemsJson(items, servers) {
    return JSON.stringify({
      apiVersion: 3,
      errorCode: hatohol.HTERR_OK,
      items: items ? items : [],
      servers: servers ? servers : {}
    });
  }

  function historyJson(history) {
    return JSON.stringify({
      apiVersion: 3,
      errorCode: hatohol.HTERR_OK,
      history: history ? history : [],
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

  function respondUserConfig(configJson) {
    var request = this.requests[0];
    if (!configJson)
      configJson = '{}';
    request.respond(200, { "Content-Type": "application/json" },
                    configJson);
  }

  function respondItems(itemsJson) {
    var request = this.requests[1];
    request.respond(200, { "Content-Type": "application/json" },
                    itemsJson);
  }

  function respondHistory(historyJson) {
    var request = this.requests[2];
    request.respond(200, { "Content-Type": "application/json" },
                    historyJson);
  }

  function respond(configJson, itemsJson, historyJson) {
    respondUserConfig(configJson);
    respondItems(itemsJson);
    respondItems(historyJson);
  }
  
  beforeEach(function(done) {
    var contentId = "main";
    var setupFixture = function() {
      $("#" + TEST_FIXTURE_ID).append($("<div>", { id: contentId }));
      $("#" + contentId).html(viewHTML);
      fakeAjax();
      done();
    };

    $('body').append($('<div>', { id: TEST_FIXTURE_ID }));

    if (viewHTML) {
      setupFixture();
    } else {
      var iframe = $("<iframe>", {
        id: "fixtureFrame",
        src: "../../ajax_history?start=false",
        load: function() {
          viewHTML = $("#" + contentId, this.contentDocument).html();
          setupFixture();
        }
      });
      $("#" + TEST_FIXTURE_ID).append(iframe);
    }
  });

  afterEach(function() {
    restoreAjax();
    $("#" + TEST_FIXTURE_ID).remove();
  });

  it('new', function() {
    var view = new HistoryView($('#' + TEST_FIXTURE_ID).get(0));
    expect(view instanceof HistoryView).to.be(true);
  });
});
