describe('HatoholTracer', function() {
  // NOTE: In the actual use, a global object hatoholTracer
  //       is supposed to be used.
  it('add listener', function(done) {
    var tracer = new HatoholTracer();
    tracer.addListener(HatoholTracePoint.PRE_HREF_CHANGE, function(){});
    done();
  });

  it('invoke registered lister', function(done) {
    function cb(params) {
      expect(params.foo).to.be(1);
      expect(params.goo).to.be(-2);
      done();
    };
    var tracer = new HatoholTracer();
    var tracePointId = HatoholTracePoint.PRE_HREF_CHANGE;
    tracer.addListener(tracePointId, cb);
    tracer.pass(tracePointId, {foo:1, goo:-2});
  });
});

