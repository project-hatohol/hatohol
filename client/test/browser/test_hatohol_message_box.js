describe('HatoholMessageBox', function() {
  it('passes only the first (message) argument', function(done) {
    var msg = "Test message.";
    HatoholDialogObserver.registerCreatedCallback(function(id, obj) {
      if (!("getDefaultId" in obj))
        return;
      if (id != obj.getDefaultId())
        return;

      expect(obj.getMessage()).to.be(msg);

      // title bar
      expect(obj.isTitleBarVisible()).to.be(false);
      expect(obj.getTitleString()).to.be(obj.getDefaultTitleString());

      // button
      var buttons = obj.getButtons();
      expect(buttons).to.have.length(1);
      var button = buttons[0];
      expect(button.text).to.be(obj.getDefaultButtonLabel());

      done();
    });
    var msgbox = new HatoholMessageBox(msg);
  });
});
