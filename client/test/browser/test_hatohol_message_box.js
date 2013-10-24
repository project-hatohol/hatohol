describe('HatoholMessageBox', function() {
  it('passes only the first (message) argument', function(done) {
    var msg = "Test message.";
    HatoholDialogObserver.registerCreatedCallback(function(id, obj) {
      if (!("getDefaultId" in obj))
        return;
      if (id != obj.getDefaultId())
        return;

      expect(obj.getMessage()).to.be(msg);
      expect(obj.isTitleBarVisible()).to.be(false);
      done();
    });
    var msgbox = new HatoholMessageBox(msg);
  });
});
