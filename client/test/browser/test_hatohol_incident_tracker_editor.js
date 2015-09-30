describe('HatoholIncidentTrackerEditor', function() {
  var editor;

  beforeEach(function() {
    editor = undefined;
  });

  afterEach(function() {
    if (editor)
      editor.closeDialog();
  });

  it('new', function() {
    var expectedId = "#incident-tracker-editor";
    editor = new HatoholIncidentTrackerEditor({});
    expect(editor).not.to.be(undefined);
    expect($(expectedId)).to.have.length(1);
    var buttons = $(expectedId).dialog("option", "buttons");
    expect(buttons).to.have.length(2);
    expect(buttons[0].text).to.be(gettext("ADD"));
    expect(buttons[1].text).to.be(gettext("CANCEL"));
  });

  it('change to Hatohol incident tracker type', function() {
    var expectedId = "#incident-tracker-editor";
    var editFormsArea = "#editIncidentTrackerArea";
    editor = new HatoholIncidentTrackerEditor({});
    expect(editor).not.to.be(undefined);
    expect($(expectedId)).to.have.length(1);
    var buttons = $(expectedId).dialog("option", "buttons");
    $("#selectIncidentTrackerType").val(gettext("Hatohol")).change();
    // Make synchronous
    setTimeout(function() {
      expect($(editFormsArea).css('display')).to.be("none");
    }, 0);
  });
});
