describe('HatoholUserRolesEditor', function() {
  var editor;

  beforeEach(function() {
    editor = undefined;
  });

  afterEach(function() {
    if (editor)
      editor.closeDialog();
  });

  it('new', function() {
    var expectedId = "#user-roles-editor";
    editor = new HatoholUserRolesEditor({});
    expect(editor).not.to.be(undefined);
    expect($(expectedId)).to.have.length(1);
    var buttons = $(expectedId).dialog("option", "buttons");
    expect(buttons).to.have.length(1);
    expect(buttons[0].text).to.be(gettext("CLOSE"));
  });
});
