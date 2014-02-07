describe('HatoholUserEditDialog', function() {
  var dialog;

  beforeEach(function() {
    dialog = undefined;
  });

  afterEach(function() {
    if (dialog)
      dialog.closeDialog();
  });

  function getAdminUserData() {
    return {
      "userId": 2,
      "name": "Administrator",
      "flags": hatohol.ALL_PRIVILEGES
    };
  };

  it('new with empty params', function() {
    var expectedId = "#user-edit-dialog";
    dialog = new HatoholUserEditDialog({});
    expect(dialog).not.to.be(undefined);
    expect($(expectedId)).to.have.length(1);
    var buttons = $(expectedId).dialog("option", "buttons");
    expect(buttons).to.have.length(2);
    expect(buttons[0].text).to.be(gettext("ADD"));

    // check initial values
    expect($("#editUserName").val()).to.be.empty();
    expect($("#editPassword").val()).to.be.empty();
    expect($("#selectUserRole").val()).eql(hatohol.NONE_PRIVILEGE);
  });

  it('new with admin user data', function() {
    var adminUser = getAdminUserData();
    var expectedId = "#user-edit-dialog";
    dialog = new HatoholUserEditDialog({
      targetUser: adminUser
    });
    expect(dialog).not.to.be(undefined);
    expect($(expectedId)).to.have.length(1);
    var buttons = $(expectedId).dialog("option", "buttons");
    expect(buttons).to.have.length(2);
    expect(buttons[0].text).to.be(gettext("APPLY"));

    // check initial values
    expect($("#editUserName").val()).to.be(adminUser.name);
    expect($("#editPassword").val()).to.be.empty();
    expect($("#selectUserRole").val()).eql(adminUser.flags);
  });

  it('click apply button', function() {
    var adminUser = getAdminUserData();
    var connectorStart = function(params) {
      var expectedPutData = getAdminUserData();
      delete expectedPutData["userId"];
      expect(params.url).to.be("/user/2");
      expect(params.request).to.be("PUT");
      expect(params.data).to.eql(expectedPutData);
    };
    dialog = new HatoholUserEditDialog({
      targetUser: adminUser
    });
    var buttons = $(dialog.dialogId).dialog("option", "buttons");
    var stub = sinon.stub(HatoholConnector.prototype, "start", connectorStart);
    buttons[0].click();
    stub.restore();
  });
});
