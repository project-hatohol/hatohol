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

  function getOperator() {
    var operator = {
      "userId": 3,
      "name": "foobar",
      "flags": hatohol.ALL_PRIVILEGES
    };
    return new HatoholUserProfile(operator);
  }

  function itemsJson(items, servers) {
    return JSON.stringify({
      apiVersion: hatohol.FACE_REST_API_VERSION,
      errorCode: hatohol.HTERR_OK,
      items: items ? items : defaultItems,
      servers: servers ? servers : defaultServers
    });
  }

  function historyJson(history) {
    return JSON.stringify({
      apiVersion: hatohol.FACE_REST_API_VERSION,
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

  it('load history with time range', function() {
    var now = new Date();
    var endTime = Math.floor(now.getTime() / 1000);
    var beginTime = endTime - 60 * 60;
    var query = "serverId=1&hostId=10101&itemId=1" +
      "&beginTime=" + beginTime + "&endTime=" + endTime;
    var view = new HistoryView(getOperator(),
                               { query: query });
    respond(itemsJson(), historyJson());
    expect(view).to.be.a(HistoryView);
    expect(requests[0].url).to.be(
      "/tunnel/item?serverId=1&hostId=10101&itemId=1");
    expect(requests[1].url).to.be(
      "/tunnel/history?serverId=1&hostId=10101&itemId=1" +
        "&beginTime=" + beginTime + "&endTime=" + endTime);
  });

  it('load history without time range', function() {
    var now = 60 * 60 * 24;
    var clock = sinon.useFakeTimers(now * 1000, "Date");
    var endTime = now;
    var beginTime = endTime - 60 * 60 * 6;
    var query = "serverId=1&hostId=10101&itemId=1";
    var view = new HistoryView(getOperator(),
                               { query: query });
    respond(itemsJson(), historyJson());
    expect(view).to.be.a(HistoryView);
    expect(requests[0].url).to.be(
      "/tunnel/item?serverId=1&hostId=10101&itemId=1");
    expect(requests[1].url).to.be(
      "/tunnel/history?serverId=1&hostId=10101&itemId=1" +
        "&endTime=" + endTime + "&beginTime=" + beginTime);
    clock.restore();
  });

  it('format graph data', function() {
    var query = "serverId=1&hostId=10101&itemId=1";
    var view = new HistoryView(getOperator(),
                               { query: query });
    var expected = [{
      data: [
        [1415586892182, "97.8568"],
        [1415586952317, "97.4699"],
        [1415587012454, "97.1620"],
        [1415587072551, "99.3657"],
        [1415587132645, "93.3277"],
      ],
      yaxis: 1
    }];
    respond(itemsJson(), historyJson());
    expect(view.graph.plotData).eql(expected);
  });

  it('parse multiple items query', function() {
    var expected = [
      {
        serverId: 1,
        hostId: 2,
        itemId: 3,
        beginTime: 4,
        endTime: 5,
      },
      {
        serverId: 6,
        hostId: 7,
        itemId: 8,
        beginTime: 9,
        endTime: 10
      },
      {
        serverId: 11,
        hostId: 12,
        itemId: 13,
        beginTime: 14,
        endTime: 15
      }
    ];
    var queryObj = $.extend({
      histories: [expected[1], expected[2]]
    }, expected[0]);
    var query = $.param(queryObj);
    var actual = HistoryView.prototype.parseGraphItems(query);
    expect(expected).eql(actual);
  });

  it('get config from item selector', function() {
    var view = new HistoryView(getOperator());
    var selector = new HatoholItemSelector({ view: view });
    var item1 = $.extend({}, defaultItems[0]);
    var item2 = $.extend({}, item1, { id: 2 });
    var hostgroupId2 = 12;
    selector.appendItem(item1, defaultServers);
    selector.appendItem(item2, defaultServers, hostgroupId2);
    expect(selector.getConfig()).eql({
      histories: [
        {
          serverId: 1,
          hostId: 10101,
          hostgroupId: undefined,
          itemId: 1,
        },
        {
          serverId: 1,
          hostId: 10101,
          hostgroupId: 12,
          itemId: 2,
        },
      ]
    });
  });
});
