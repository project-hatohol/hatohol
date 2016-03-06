// before_close.js

(function(){
	"use strict";
	var f = true;
	function beforeunloadFlgToggle(){
		f = false;
		setTimeout(function(){f = true;},100);
	}
	if(!/^\/$/.test(location.pathname)){
		$(document).on('click',beforeunloadFlgToggle);
		
		$(window).on("beforeunload",function(){
		   if(f && !$('#hatohol_login_dialog').is(':visible')){
			   return "Hatoholがシステム監視中です。\nこのページを移動、または閉じようとしています。";
		   }
		});
	}
})();
