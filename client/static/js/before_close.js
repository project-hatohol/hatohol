(function() {
  "use strict";
  var isTransitAllowed = false;

  $(window).on("beforeunload",function(){
    if (isTransitAllowed)
      return;
    return gettext("Hatohol is running to monitor system(s).\n"\
                   "You are trying to leave or close this page.");
  });

  hatoholTracer.addListener(HatoholTracePoint.PRE_HREF_CHANGE, function() {
    isTransitAllowed = true;
  });
})();
