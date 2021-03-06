describe('ServerView', function() {
  var TEST_FIXTURE_ID = 'serversViewFixture';
  var serversViewHTML;
  var defaultServers = [
    {
      "id": 1,
      "type": hatohol.MONITORING_SYSTEM_HAPI2,
      "hostName": "Test Zabbix",
      "ipAddress": "",
      "nickname": "Test Zabbix",
      "port": 0,
      "pollinInterval": 30,
      "retryInterval": 10,
      "userName": "TestZabbixUser",
      "passowrd": "zabbix",
      "dbName": "",
      "passiveMode": false,
      "baseURL": "http://127.0.1.1/zabbix/api_jsonrpc.php",
      "brokerURL": "amqp://test_user:test_password@localhost:5673/test",
      "staticQueueAddress": "",
      "tlsCertificatePath": "",
      "tlsKeyPath": "",
      "tlsCACertificatePath": "",
      "tlsEnableVerify": "",
      "uuid": "8e632c14-d1f7-11e4-8350-d43d7e3146fb"
    },
    {
      "id": 2,
      "type": hatohol.MONITORING_SYSTEM_HAPI2,
      "hostName": "Test Nagios",
      "ipAddress": "",
      "nickname": "Test Nagios",
      "port": 0,
      "pollinInterval": 60,
      "retryInterval": 10,
      "userName": "TestNagiosUser",
      "passowrd": "nagiosadministrator",
      "dbName": "",
      "passiveMode": false,
      "baseURL": "127.0.1.1/ndoutils",
      "brokerURL": "amqp://test_user:test_password@localhost:5673/test",
      "staticQueueAddress": "",
      "tlsCertificatePath": "",
      "tlsKeyPath": "",
      "tlsCACertificatePath": "",
      "tlsEnableVerify": "",
      "uuid": "902d955c-d1f7-11e4-80f9-d43d7e3146fb",
    },
    {
      "id": 3,
      "type": hatohol.MONITORING_SYSTEM_HAPI2,
      "hostName": "Test Nagios2",
      "ipAddress": "",
      "nickname": "Test Nagios2",
      "port": 0,
      "pollinInterval": 60,
      "retryInterval": 10,
      "userName": "TestNagiosUser2",
      "passowrd": "nagiosadministrator",
      "dbName": "",
      "passiveMode": false,
      "baseURL": "10.0.0.1/ndoutils",
      "staticQueueAddress": "",
      "tlsCertificatePath": "",
      "tlsKeyPath": "",
      "tlsCACertificatePath": "",
      "tlsEnableVerify": "",
      "uuid": "902d955c-d1f7-11e4-80f9-d43d7e3146fb",
    },
  ];

  function getServersJson(servers) {
    return JSON.stringify({
      apiVersion: hatohol.FACE_REST_API_VERSION,
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
        $('#loaderFrame').remove();
      }
    });
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

  function expectDeleteButtonVisibility(operator, expectedVisibility) {
    var userProfile = new HatoholUserProfile(operator);
    var view = new ServersView(userProfile);
    respond();

    var deleteButton = $('#delete-server-button');
    var checkboxes = $('.delete-selector .selectcheckbox');
    expect(deleteButton).to.have.length(1);
    expect(checkboxes).to.have.length(3);
    expect(deleteButton.is(":visible")).to.be(expectedVisibility);
    expect(checkboxes.is(":visible")).to.be(expectedVisibility);
  }

  function expectEditButtonVisibility(operator, expectedVisibility) {
    var userProfile = new HatoholUserProfile(operator);
    var view = new ServersView(userProfile);
    respond();

    var editButton = $('#edit-server1');
    var editColumn = $('td.edit-server-column');
    expect(editButton).to.have.length(1);
    expect(editColumn).to.have.length(3);
    expect(editButton.is(":visible")).to.be(expectedVisibility);
    expect(editColumn.is(":visible")).to.be(expectedVisibility);
  }

  function expectLinkVisibility(operator, expectedVisibleLinkNums) {
    var userProfile = new HatoholUserProfile(operator);
    var view = new ServersView(userProfile);
    respond();

    var nicknameColumn = $('td.server-url-link');
    expect(nicknameColumn).to.have.length(expectedVisibleLinkNums);
  }

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
    var expected = true;
    expectDeleteButtonVisibility(operator, expected);
  });

  it('with delete_all privilege', function() {
    var operator = {
      "userId": 2,
      "name": "guest",
      "flags": (1 << hatohol.OPPRVLG_GET_ALL_SERVER |
                1 << hatohol.OPPRVLG_DELETE_ALL_SERVER)
    };
    var expected = true;
    expectDeleteButtonVisibility(operator, expected);
  });

  it('with no delete privilege', function() {
    var operator = {
      "userId": 2,
      "name": "guest",
      "flags": 0
    };

    // issue #1108 & #1122
    // Always show them for "RELOAD ALL TRIGGERS FROM SERVER" button
    //var expected = false;
    //expectDeleteButtonVisibility(operator, expected);

    var userProfile = new HatoholUserProfile(operator);
    var view = new ServersView(userProfile);
    respond();

    var deleteButton = $('#delete-server-button');
    var checkboxes = $('.delete-selector .selectcheckbox');
    expect(deleteButton).to.have.length(1);
    expect(checkboxes).to.have.length(3);
    expect(deleteButton.is(":visible")).to.be(false);
    expect(checkboxes.is(":visible")).to.be(true);
  });

  it('with update privilege', function() {
    var operator = {
      "userId": 1,
      "name": "admin",
      "flags": (1 << hatohol.OPPRVLG_UPDATE_ALL_SERVER |
                1 << hatohol.OPPRVLG_UPDATE_SERVER)
    };
    var expected = true;
    expectEditButtonVisibility(operator, expected);
  });

  it('with all update privilege', function() {
    var operator = {
      "userId": 2,
      "name": "guest",
      "flags": (1 << hatohol.OPPRVLG_UPDATE_ALL_SERVER)
    };
    var expected = true;
    expectEditButtonVisibility(operator, expected);
  });

  it('without update privilege: edit button visibility', function() {
    var operator = {
      "userId": 2,
      "name": "guest",
      "flags": 0
    };
    var expected = false;
    expectEditButtonVisibility(operator, expected);
  });

  it('without update privilege: link visibility', function() {
    var operator = {
      "userId": 2,
      "name": "guest",
      "flags": 0
    };
    // Currently js.plugins/hap2-nagios-ndoutils.js doesn't have getTopURL()
    // So only the first server makes a link.
    var expectedVisibleLinkNums = 1;
    expectLinkVisibility(operator, expectedVisibleLinkNums);
  });
});
