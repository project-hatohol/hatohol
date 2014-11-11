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
  var defaultHistory = [
    { "value":"97.8568", "clock":1415586892, "ns":182297994 },
    { "value":"97.4699", "clock":1415586952, "ns":317020796 },
    { "value":"97.1620", "clock":1415587012, "ns":454507050 },
    { "value":"99.3657", "clock":1415587072, "ns":551434487 },
    { "value":"93.3277", "clock":1415587132, "ns":645202531 }
  ];

  function itemsJson(items, servers) {
    return JSON.stringify({
      apiVersion: 3,
      errorCode: hatohol.HTERR_OK,
      items: items ? items : defaultItems,
      servers: servers ? servers : defaultServers
    });
  }

  function historyJson(history) {
    return JSON.stringify({
      apiVersion: 3,
      errorCode: hatohol.HTERR_OK,
      history: history ? history : defaultHistory,
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

  function respond(itemsJson, historyJson) {
    var header = { "Content-Type": "application/json" };
    this.requests[0].respond(200, header, itemsJson);
    this.requests[1].respond(200, header, historyJson);
  }
  
  beforeEach(function(done) {
    var contentId = "main";
    var setupFixture = function() {
      $("#" + TEST_FIXTURE_ID).append($("<div>", { id: contentId }));
      $("#" + contentId).html(viewHTML);
      fakeAjax();
      done();
    };
    var loadFixture = function() {
      $("#" + TEST_FIXTURE_ID).append($("<iframe>", {
        id: "fixtureFrame",
        src: "../../ajax_history?start=false",
        load: function() {
          viewHTML = $("#" + contentId, this.contentDocument).html();
          setupFixture();
        }
      }));
    };

    $('body').append($('<div>', { id: TEST_FIXTURE_ID }));

    if (viewHTML)
      setupFixture();
    else
      loadFixture();
  });

  afterEach(function() {
    restoreAjax();
    $("#" + TEST_FIXTURE_ID).remove();
  });

  it('new', function() {
    var query = "serverId=1&hostId=10101&itemId=1";
    var view = new HistoryView($('#' + TEST_FIXTURE_ID).get(0),
                               { query: query });
    respond(itemsJson(), historyJson());
    expect(view).to.be.a(HistoryView);
    expect(requests[0].url).to.be(
      "/tunnel/item?serverId=1&hostId=10101&itemId=1");
    expect(requests[1].url).to.be(
      "/tunnel/history?serverId=1&hostId=10101&itemId=1");
  });
});
