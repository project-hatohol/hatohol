describe('HatoholServerEditDialog', function() {
  var dialog;

  beforeEach(function() {
    dialog = undefined;
  });

  afterEach(function() {
    if (dialog)
      dialog.closeDialog();
  });

  it('new with empty params', function() {
    var expectedId = "#server-edit-dialog";
    dialog = new HatoholServerEditDialog({});
    expect(dialog).not.to.be(undefined);
    expect($(expectedId)).to.have.length(1);
    var buttons = $(expectedId).dialog("option", "buttons");
    expect(buttons).to.have.length(2);
    expect(buttons[0].text).to.be(gettext("ADD"));

    // check initial values
    expect($("#selectServerType").val()).to.be("0");
    expect($("#inputNickName").val()).to.be.empty();
    expect($("#inputHostName").val()).to.be.empty();
    expect($("#inputIpAddress").val()).to.be.empty();
    expect($("#inputPort").val()).to.be("80");
    expect($("#inputUserName").val()).to.be.empty();
    expect($("#inputPassword").val()).to.be.empty();
    expect($("#inputDbName").val()).to.be.empty();
    expect($("#inputPollingInterval").val()).to.be("30");
    expect($("#inputRetryInterval").val()).to.be("10");

    // check initial state
    expect($("#dbNameArea").css("display")).to.be("none");
  });

  it('new with a nagios server', function() {
    var expectedId = "#server-edit-dialog";
    var server = {
      id: 1,
      type: hatohol.MONITORING_SYSTEM_NAGIOS,
      hostName: "localhost",
      ipAddress: "127.0.0.1",
      nickname: "MySelf",
      port: 3306,
      pollingInterval: 20,
      retryInterval: 15,
      userName: "nagios",
      password: "soigan",
      dbName: "nagios-db"
    };
    dialog = new HatoholServerEditDialog({
      targetServer: server
    });
    expect(dialog).not.to.be(undefined);
    expect($(expectedId)).to.have.length(1);
    var buttons = $(expectedId).dialog("option", "buttons");
    expect(buttons).to.have.length(2);
    expect(buttons[0].text).to.be(gettext("APPLY"));

    // check initial values
    expect($("#selectServerType").val()).eql(server.type);
    expect($("#inputNickName").val()).to.be(server.nickname);
    expect($("#inputHostName").val()).to.be(server.hostName);
    expect($("#inputIpAddress").val()).to.be(server.ipAddress);
    expect($("#inputPort").val()).eql(server.port);
    expect($("#inputUserName").val()).to.be(server.userName);
    expect($("#inputPassword").val()).to.be(server.password);
    expect($("#inputDbName").val()).to.be(server.dbName);
    expect($("#inputPollingInterval").val()).eql(server.pollingInterval);
    expect($("#inputRetryInterval").val()).eql(server.retryInterval);

    // check initial state
    expect($("#dbNameArea").css("display")).not.to.be("none");
  });

  it('DB name visibility', function() {
    dialog = new HatoholServerEditDialog({});
    expect($("#dbNameArea").css("display")).to.be("none");
    $("#selectServerType").val(hatohol.MONITORING_SYSTEM_NAGIOS).change();
    expect($("#dbNameArea").css("display")).not.to.be("none");
  });
});
