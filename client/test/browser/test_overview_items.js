describe('OverviewItems', function() {
  var TEST_FIXTURE_ID = 'overviewItemsFixture';
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

  function respondItems(itemsJson) {
    var i = 0;
    var request = this.requests[i++];
    request.respond(200, { "Content-Type": "application/json" },
                    itemsJson);
  }

  function respond(itemsJson, configJson) {
    respondItems(itemsJson);
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
        src: "../../ajax_overview_items?start=false",
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

  it('new with empty data', function() {
    var view = new OverviewItems($('#' + TEST_FIXTURE_ID).get(0));
    respond(itemsJson());
    var heads = $('div#' + TEST_FIXTURE_ID + ' h2');
    expect(heads).to.have.length(1);
    expect($('#table')).to.have.length(1);
  });
});
