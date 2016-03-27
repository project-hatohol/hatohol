(function() {
	"use strict";
	var beforeUnloadFlag = true;

  $(document).on('click', 'a', function(e) {
    defendWindowClose(e, $(this));
  });

  function defendWindowClose(e, $this) {
    var target = $this.attr('target');
    var href = $this.attr('href');
    if(
      !(
        e.shiftKey ||
        e.ctrlKey ||
        e.altKey ||
        typeof href === "undefined" ||
        /#/.test(href)
      ) &&
      (
        typeof target === "undefined" ||
        target === "" ||
        target.toLowerCase() === "_self" ||
        target.toLowerCase() === "_top" ||
        target === window.name
      )
    ) {
      beforeUnloadFlag = false;
    }
  }

  $(window).on("beforeunload",function(){
    if(beforeUnloadFlag && !$('#hatohol_login_dialog').is(':visible')){
      return gettext("Hatohol is running to monitor system(s).\n" +
      "You are trying to transition or close this page.");
    }
  });
})();
