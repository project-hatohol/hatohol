// before_close.js

(function(){
	"use strict";
	var beforeunloadFlug = true;
	function beforeunloadFlgToggle(){
		beforeunloadFlug = false;
		setTimeout(function(){beforeunloadFlug = true;},1);
	}
	if(!/^\/$/.test(location.pathname)){
		$(document).on('click',beforeunloadFlgToggle);
		
		$(window).on("beforeunload",function(){
		   if(beforeunloadFlug && !$('#hatohol_login_dialog').is(':visible')){
			   return gettext("Hatohol is in system monitoring.\nYou are trying to transition or close this page.");
		   }
		});
	}
})();
