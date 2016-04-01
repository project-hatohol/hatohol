(function() {
	"use strict";
  var isTransitAllowed = false;

  $(window).on("beforeunload",function(){
    console.log("beforeunload !!!!!!!!!!!!!!: flag: " + isTransitAllowed);
    if (isTransitAllowed)
      return;
    return gettext("Hatohol is running to monitor system(s).\n" +
    "You are trying to transition or close this page.");
  });

  hatoholTracer.addListener(HatoholTracePoint.PRE_HREF_CHANGE, function() {
    console.log("Receive !!!!: flag: " + isTransitAllowed);
    isTransitAllowed = true;
  });
})();
