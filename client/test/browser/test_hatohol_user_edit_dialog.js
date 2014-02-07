describe('HatoholUserEditDialog', function() {
  var dialog;

  beforeEach(function() {
    dialog = undefined;
  });

  afterEach(function() {
    if (dialog)
      dialog.closeDialog();
  });

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
    var adminUser = {
      "name": "Administrator",
      "flags": hatohol.ALL_PRIVILEGES
    };
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
});
