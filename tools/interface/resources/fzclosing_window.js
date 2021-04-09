// fzclosing_window.js - Randal A. Koene, 20210409
// Formalizer standard auto-closing window (if opened by script).
// Note: Load this in a <script> tag at end of <body> section or within $("document").ready(function(){ ... }); as when in <head> section.

var will_close = true;
var countdownStart = 5;
var countdownFrom;
var closing_button_text;
var closing_button_url;
var stp;
function do_if_opened_by_script(button_text, alt_text, alt_url) {
    if(window.opener === null) {
        //window.location.href = '<YOUR_DEFAULT_URL>';
        x = document.getElementById(id);
        x.innerHTML = alt_text;
        closing_button_url = alt_url;
    } else {
        closing_button_text = button_text;
        Start_Closing_Countdown('closing_countdown');
    }
}
function Start_Closing_Countdown(id) {
    countdownFrom = countdownStart;
    Show_Countdown(id)
    stp = setInterval("CountDownTimer('"+id+"')",1000);
}
function CountDownTimer(id) {
    if (countdownFrom == 0) {
        clearInterval(stp);
        window.close(); 
    } else {
        Show_Countdown(id);
        countdownFrom--;
    }
}
function Show_Countdown(id) {
    var x;
    //var cntText = "Page closing in "+countdownFrom+" seconds: "+"&#x25A0;".repeat(countdownFrom);
    var cntText = closing_button_text+" (closing: "+"&#9632;".repeat(countdownFrom)+"&#9633;".repeat(countdownStart-countdownFrom)+")";
    if (document.getElementById) {
        x = document.getElementById(id);
        x.innerHTML = cntText;
    } else {
        if (document.all) {
            x = document.all[id];
            x.innerHTML = cntText;
        }
    }
}
function Keep_or_Close_Page(id) {
    if(window.opener === null) {
        window.location.href = closing_button_url;
    } else {
        if (will_close) {
            countdownFrom = 5;
            clearInterval(stp);
            x = document.getElementById(id);
            x.innerHTML = "Close Page";
            will_close = false;
        } else {
            window.close();
        }
    }  
}
