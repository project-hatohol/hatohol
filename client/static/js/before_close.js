// before_close.js

(function(){
	"use strict";
	var beforeUnloadFlag = true;
	function hookUnloadFlagAsync(){
		beforeUnloadFlag = false;
		setTimeout(function(){beforeUnloadFlag = true;},1);
	}
	if(!/^\/$/.test(location.pathname)){
		$(document).on('click',hookUnloadFlagAsync);
		
		$(window).on("beforeunload",function(){
		   if(beforeUnloadFlag && !$('#hatohol_login_dialog').is(':visible')){
				return gettext("Hatohol is running to monitor system(s).\n" +
				"You are trying to transition or close this page.");
		   }
		});
	}
})();
