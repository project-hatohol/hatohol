/**
 * JQuery shiftcheckbox plugin enhanced to two or more checkbox groups.
 *
 * shiftcheckbox provides a simpler and faster way to select/unselect multiple checkboxes within a given range with just two clicks.
 * Inspired from GMail checkbox functionality
 *
 * Just call $('.<class-name>').shiftcheckbox() in $(document).ready
 *
 * @name shiftcheckbox
 * @type jquery
 * @cat Plugin/Form
 * @return JQuery
 *
 * @URL http://www.sanisoft.com/blog/2009/07/02/jquery-shiftcheckbox-plugin
 *
 * Copyright (c) 2009 Aditya Mooley <adityamooley@sanisoft.com>
 * Dual licensed under the MIT (MIT-LICENSE.txt) and GPL (GPL-LICENSE.txt) licenses
 *
 *
 * Powered by en-pc service <info@en-pc.jp>
 * Copyright (c) 2010 en-pc service <info@en-pc.jp>

 */

(function ($) {

    var selectorStr = [];

    $.fn.shiftcheckbox = function()
    {
        var prevChecked = [];
        var classname = this.attr('class');
        var endChecked  = [];  // wheather checked 'end point'
        var startStatus = [];  // status of 'start point' (checked or not)
        prevChecked[classname] = null;
        selectorStr[classname] = this;
        endChecked[classname]  = null;
        startStatus[classname] = null;

        this.bind("click", function(event) {
            var val = $(this).find('[type=checkbox]').val();
            var checkStatus = $(this).find('[type=checkbox]').prop('checked');
            if ( checkStatus == undefined ) checkStatus = false;

            //get the checkbox number which the user has checked

            //check whether user has pressed shift
            if (event.shiftKey) {
                if (prevChecked[classname] != null) {  // if check 'end point' with ShiftKey

                    //get the current checkbox number
                    var ind = 0, found = 0, currentChecked;
                    currentChecked = getSelected(val,classname);

                    if (currentChecked < prevChecked[classname]) {
                        $(selectorStr[classname]).each(function(i) {
                            if (i >= currentChecked && i <= prevChecked[classname]) {
                                $(this).find('[type=checkbox]').prop('checked' , startStatus[classname]);
                            }
                        });
                    } else {
                        $(selectorStr[classname]).each(function(i) {
                            if (i >= prevChecked[classname] && i <= currentChecked) {
                                $(this).find('[type=checkbox]').prop('checked' , startStatus[classname]);
                            }
                        });
                    }

                    prevChecked[classname] = currentChecked;
                    endChecked[classname]  = true;
                }
                else {                                 // if check 'start point' with ShiftKey
                    prevChecked[classname] = getSelected(val,classname);
                    endChecked[classname]  = null;
                    startStatus[classname] = checkStatus;
                }
            } else {                                   // considered to be 'start point'(if not press ShiftKey)
                    prevChecked[classname] = getSelected(val,classname);
                    endChecked[classname]  = null;
                    startStatus[classname] = checkStatus;
            }
        });
        this.bind("keyup", function(event) {
            if (endChecked[classname]) {
                    prevChecked[classname] = null;
            }
        });

    };


    function getSelected(val,classname)
    {
        var ind = 0, found = 0, checkedIndex;

        $(selectorStr[classname]).each(function(i) {
            if (val == $(this).find('[type=checkbox]').val() && found != 1) {
                checkedIndex = ind;
                found = 1;
            }
            ind++;
        });

        return checkedIndex;
    };
})(jQuery);
