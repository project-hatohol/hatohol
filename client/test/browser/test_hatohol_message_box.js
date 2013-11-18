describe('HatoholMessageBox', function() {

  function isTargetObject(id, obj) {
    if (!("getDefaultId" in obj))
      return false;
    if (obj.getDefaultId() != id)
      return false;
    return true;
  }

  function checkResult(id, obj, expected) {
    if (!("getDefaultId" in obj))
      return;
    if (("id" in expected) && id == expected.id)
      ;
    else if (id != obj.getDefaultId())
      return;

    // message
    expect(obj.getMessage()).to.be(expected.msg);

    // title bar
    expect(obj.isTitleBarVisible()).to.be(expected.titleVisible);
    expect(obj.getTitleString()).to.be(expected.titleString);

    // button
    var buttons = obj.getButtons();
    expect(buttons).to.have.length(expected.buttonLabels.length);
    for (var i = 0; i < expected.buttonLabels.length; i++) {
      var button = buttons[i];
      expect(button.text).to.be(expected.buttonLabels[i]);
    }
  }

  function checkNoYesButton(buttons) {
    expect(buttons).to.be.an('array'); 
    expect(buttons).to.have.length(2);
    expect(buttons[0].text).to.be(gettext("NO"));
    expect(buttons[1].text).to.be(gettext("YES"));
  }

  // -------------------------------------------------------------------------
  // Test cases
  // -------------------------------------------------------------------------
  var msgbox;

  beforeEach(function(done) {
    msgbox = undefined;
    done();
  });

  afterEach(function(done) {
    HatoholDialogObserver.reset();
    if (msgbox)
      msgbox.destroy();
    done();
  });

  it('passes only the first (message) argument', function(done) {
    var msg = "Test message.";
    HatoholDialogObserver.registerCreatedCallback(function(id, obj) {
      checkResult(id, obj, {
        msg: msg,
        titleVisible: false,
        titleString: obj.getDefaultTitleString(),
        buttonLabels: [obj.getDefaultButtonLabel()],
      });
      done();
    });
    msgbox = new HatoholMessageBox(msg);
  });

  it('specify the title', function(done) {
    var msg = "Test message.";
    var param = {title: "Title Teeeeeest"};
    HatoholDialogObserver.registerCreatedCallback(function(id, obj) {
      checkResult(id, obj, {
        msg: msg,
        titleVisible: true,
        titleString: param.title,
        buttonLabels: [obj.getDefaultButtonLabel()],
      });
      done();
    });
    msgbox = new HatoholMessageBox(msg, param);
  });

  it('specify the defaultButtonLabel', function(done) {
    var msg = "Test message.";
    var param = {title: "Test MessageBox", defaultButtonLabel:"<lv.vl>"};
    HatoholDialogObserver.registerCreatedCallback(function(id, obj) {
      checkResult(id, obj, {
        msg: msg,
        titleVisible: true,
        titleString: param.title,
        buttonLabels: [param.defaultButtonLabel],
      });
      done();
    });
    msgbox = new HatoholMessageBox(msg, param);
  });

  it('specify ID', function(done) {
    var msg = "Test message.";
    var customId = "HOGETA-HOGETARO-Zaemon";
    var param = {id: customId};
    HatoholDialogObserver.registerCreatedCallback(function(id, obj) {
      checkResult(id, obj, {
        id: customId,
        msg: msg,
        titleVisible: false,
        titleString: obj.getDefaultTitleString(),
        buttonLabels: [obj.getDefaultButtonLabel()],
      });
      done();
    });
    msgbox = new HatoholMessageBox(msg, param);
  });

  it('specify buttons', function(done) {
    var msg = "Test message.";
    var buttons = [{
      text: "Fish and chips", click: function(){}
    }, {
      text: "Easy come, easy go.", click: function(){}
    }, {
      text: "All is fair in love and war.", click: function(){}
    }];
    var param = {buttons: buttons};
    var expectButtonLabels = [];
    for (var i = 0; i < buttons.length; i++)
      expectButtonLabels.push(buttons[i].text);
    HatoholDialogObserver.registerCreatedCallback(function(id, obj) {
      checkResult(id, obj, {
        msg: msg,
        titleVisible: false,
        titleString: obj.getDefaultTitleString(),
        buttonLabels: expectButtonLabels,
      });
      done();
    });
    msgbox = new HatoholMessageBox(msg, param);
  });

  it('get builtin NO/YES buttons', function() {
    var buttons = HatoholMessageBox.prototype.getBuiltinNoYesButtons();
    checkNoYesButton(buttons);
  });

  it('confirm builtin NO/YES buttons are deeply copied', function() {
    var buttons = HatoholMessageBox.prototype.getBuiltinNoYesButtons();
    buttons[0].text = "annihilation engine";
    buttons = HatoholMessageBox.prototype.getBuiltinNoYesButtons();
    checkNoYesButton(buttons);
  });

  it('hatoholMsgBoxForParser on a status error', function(done) {
    HatoholDialogObserver.registerCreatedCallback(function(id, obj) {
      if (!isTargetObject(id, obj))
        return;
      var expectRe = new RegExp('^.*' + gettext('STATUS CODE') + ': ' + '.*$');
      expect(obj.getMessage()).to.match(expectRe);
      done();
    });
    var reply = {};
    var parser = new HatoholReplyParser(reply);
    hatoholMsgBoxForParser(reply, parser);
  });

  it('hatoholMsgBoxForParser on a server error', function(done) {
    HatoholDialogObserver.registerCreatedCallback(function(id, obj) {
      if (!isTargetObject(id, obj))
        return;
      var expectRe = new RegExp('^.*' + gettext('HATOHOL SERVER ERROR CODE') + ': ' + hatohol.HTERR_ERROR_TEST + '.*$');
      expect(obj.getMessage()).to.match(expectRe);
      obj.destroy();
      done();
    });
    var reply = {apiVersion: hatohol.FACE_REST_API_VERSION,
                 errorCode: hatohol.HTERR_ERROR_TEST};
    var parser = new HatoholReplyParser(reply);
    hatoholMsgBoxForParser(reply, parser);
  });
});
