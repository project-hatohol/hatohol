describe('ServerView', function() {
  var TEST_FIXTURE_ID = 'serversViewFixture';
  var serversViewHTML;
  var defaultServers = [
    {
      "id": 1,
      "type": hatohol.MONITORING_SYSTEM_ZABBIX,
      "hostName": "Test Zabbix",
      "ipAddress": "127.0.0.1",
      "nickname": "Test Zabbix",
      "port": 80,
      "pollinInterval": 30,
      "retryInterval": 10,
      "userName": "TestZabbixUser",
      "passowrd": "zabbix",
      "dbName": ""
    }
  ];

  function getServersJson(servers) {
    return JSON.stringify({
      apiVersion: 3,
      errorCode: hatohol.HTERR_OK,
      servers: servers ? servers : defaultServers,
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

  function respond(serversJson) {
    var request = this.requests[0];
    request.respond(200, { "Content-Type": "application/json" },
                    serversJson ? serversJson : getServersJson());
  }

  function loadFixture(pathFromTop, onLoad) {
    var iframe = $("<iframe>", {
      id: "loaderFrame",
      src: "../../" + pathFromTop + "?start=false",
      load: function() {
        var html = $("#main", this.contentDocument).html();
        onLoad(html);
        $('#loaderFrame').remove()
      }
    })
    $('body').append(iframe);
  }

  beforeEach(function(done) {
    $('body').append($('<div>', { id: TEST_FIXTURE_ID }));
    var setupFixture = function(html) {
      serversViewHTML = html;
      $("#" + TEST_FIXTURE_ID).append($("<div>", { id: "main" }))
      $("#main").html(serversViewHTML);
      fakeAjax();
      done();
    };

    if (serversViewHTML)
      setupFixture(serversViewHTML);
    else
      loadFixture("ajax_servers", setupFixture)
  });

  afterEach(function() {
    restoreAjax();
    $("#" + TEST_FIXTURE_ID).remove();
  });

  function checkGetStatusLabel(stat, expectMsg, expectMsgClass) {
    var pkt = {serverConnStat:{'5':{status:stat}}};
    var parser = new ServerConnStatParser(pkt);
    expect(parser.setServerId(5)).to.be(true); 
    var label = parser.getStatusLabel();
    expect(label.msg).to.be(expectMsg);
    expect(label.msgClass).to.be(expectMsgClass);
  }

  // -------------------------------------------------------------------------
  // Test cases
  // -------------------------------------------------------------------------
  it('pass an undefined packet', function() {
    var parser = new ServerConnStatParser();
    expect(parser.isBadPacket()).to.be(true); 
  });

  it('pass an undefined serverConnStat', function() {
    var pkt = {};
    var parser = new ServerConnStatParser(pkt);
    expect(parser.isBadPacket()).to.be(true); 
  });

  it('set nonexisting server id', function() {
    var pkt = {serverConnStat:{}};
    var parser = new ServerConnStatParser(pkt);
    expect(parser.setServerId(5)).to.be(false); 
  });

  it('set existing server id', function() {
    var pkt = {serverConnStat:{'5':{}}};
    var parser = new ServerConnStatParser(pkt);
    expect(parser.setServerId(5)).to.be(true); 
  });

  it('get status label before calling setServerId()', function() {
    var pkt = {serverConnStat:{'5':{}}};
    var parser = new ServerConnStatParser(pkt);
    var thrown = false;
    try {
      parser.getStatusLabel();
    } catch (e) {
      thrown = true;
      expect(e).to.be.a(Error);
    } finally {
      expect(thrown).to.be(true);
    }
  });

  it('get status label with no status member', function() {
    var pkt = {serverConnStat:{'5':{}}};
    var parser = new ServerConnStatParser(pkt);
    expect(parser.setServerId(5)).to.be(true); 
    expect(parser.getStatusLabel()).to.be("N/A");
  });

  it('get status label at initial state', function() {
    checkGetStatusLabel(hatohol.ARM_WORK_STAT_INIT,
                        "Initial State", "text-warning");
  });

  it('get status label at OK state', function() {
    checkGetStatusLabel(hatohol.ARM_WORK_STAT_OK, "OK", "text-success");
  });

  it('get status label at Error state', function() {
    checkGetStatusLabel(hatohol.ARM_WORK_STAT_FAILURE, "Error", "text-error");
  });

  it('get status label at unknown state', function() {
    checkGetStatusLabel(12345, "Unknown:12345", "text-error");
  });

  it('get info HTML before calling setServerId', function() {
    var pkt = {serverConnStat:{'5':{}}};
    var parser = new ServerConnStatParser(pkt);
    var thrown = false;
    try {
      parser.getInfoHTML();
    } catch (e) {
      thrown = true;
      expect(e).to.be.a(Error);
    } finally {
      expect(thrown).to.be(true);
    }
  });

  it('get info HTML', function() {
    var pkt = {serverConnStat:
      {
        '5':{
          status: hatohol.ARM_WORK_STAT_OK,
          running: 1,
          statUpdateTime: "1394444393.469501123",
          lastSuccessTime: "1394444393.469501123",
          lastFailureTime: "0.000000000",
          numUpdate: 100,
          numFailure: 5,
          failureComment: "Bouno",
        },
      }
    };
    var parser = new ServerConnStatParser(pkt);
    expect(parser.setServerId(5)).to.be(true); 
    var html = parser.getInfoHTML();
    var expectDate = parser.unixTimeToVisible("1394444393.469501123");
    var expectStr =
      gettext("Running") + ": " + gettext("Yes") + "<br>" +
      gettext("Status update time") + ": " + expectDate + "<br>" +
      gettext("Last success time") + ": " + expectDate + "<br>" + 
      gettext("Last failure time") + ": " + gettext("-") + "<br>" +
      gettext("Number of communication") + ": 100" + "<br>" +
      gettext("Number of failure") + ": 5" + "<br>" +
      gettext("Comment for the failure") + ": Bouno";
    expect(html).to.be(expectStr);
  });

  it('with delete privilege', function() {
    var operator = {
      "userId": 1,
      "name": "admin",
      "flags": (1 << hatohol.OPPRVLG_GET_ALL_SERVER |
                1 << hatohol.OPPRVLG_DELETE_SERVER)
    };
    var userProfile = new HatoholUserProfile(operator);
    var view = new ServersView(userProfile);
    respond();

    var deleteButton = $('#delete-server-button');
    var checkboxes = $('.delete-selector .selectcheckbox');
    expect(deleteButton).to.have.length(1);
    expect(checkboxes).to.have.length(1);
    expect(deleteButton.is(":visible")).to.be(true);
    expect(checkboxes.is(":visible")).to.be(true);
  });

  it('with no delete privilege', function() {
    var operator = {
      "userId": 2,
      "name": "guest",
      "flags": 0
    };
    var userProfile = new HatoholUserProfile(operator);
    var view = new ServersView(userProfile);
    respond();

    var deleteButton = $('#delete-server-button');
    var checkboxes = $('.delete-selector .selectcheckbox');
    expect(deleteButton).to.have.length(1);
    expect(checkboxes).to.have.length(1);
    expect(deleteButton.is(":visible")).to.be(false);
    expect(checkboxes.is(":visible")).to.be(false);
  });

});
