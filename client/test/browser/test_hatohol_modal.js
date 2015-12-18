describe('HatoholModal', function() {
  var modal;

  beforeEach(function() {
    modal = undefined;
  });

  afterEach(function(done) {
    if (modal)
      modal.close(done);
  });

  it('new', function() {
    var expectedId = "hatohol-modal";
    modal = new HatoholModal({id: expectedId});
    modal.show();
    expect(modal).not.to.be(undefined);
    expect($('#' + expectedId)).to.have.length(1);
  });

  it('with title', function() {
    var expectedId = "hatohol-modal";
    var title = "test title";
    modal = new HatoholModal({id: expectedId, title: title});
    modal.show();
    expect(modal).not.to.be(undefined);
    expect($('#' + expectedId)).to.have.length(1);
    expect($('.modal-title').text()).to.be(title);
  });

  it('with footer', function() {
    var expectedId = "hatohol-modal";
    var footer = "test footer";
    modal = new HatoholModal({id: expectedId, footer: footer});
    modal.show();
    expect(modal).not.to.be(undefined);
    expect($('#' + expectedId)).to.have.length(1);
    expect($('.modal-footer').text()).to.be(footer);
  });

  it('with body', function() {
    var expectedId = "hatohol-modal";
    var body = "test body";
    modal = new HatoholModal({id: expectedId, body: body});
    modal.show();
    expect(modal).not.to.be(undefined);
    expect($('#' + expectedId)).to.have.length(1);
    expect($('.modal-body').text()).to.be(body);
  });
});
