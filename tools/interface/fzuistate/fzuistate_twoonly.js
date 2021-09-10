// fzuistate.js - Randal A. Koene, 20210315
// Formalizer Web UI standard state handling
// Note: Load this in a <script> tag at end of <body> section or within $("document").ready(function(){ ... }); as when in <head> section.

// add the addnode button
var anbtnelement = document.createElement("div");
anbtnelement.innerHTML = "<button id=\"addnode\" class=\"button button2\" onclick=\"window.open('/cgi-bin/fzgraphhtml-cgi.py?edit=new','_blank');\">Add Node</button>";
document.body.prepend(anbtnelement);
// add the logtime button
var ltbtnelement = document.createElement("div");
ltbtnelement.innerHTML = "<button id=\"logtime\" class=\"button button2\" onclick=\"window.open('/cgi-bin/fzlogtime.cgi?source=nonlocal&cgivar=wrap','_blank');\">Log Time</button>";
document.body.prepend(ltbtnelement);
// add the logentry button
var lebtnelement = document.createElement("div");
lebtnelement.innerHTML = "<button id=\"logentry\" class=\"button button1\" onclick=\"window.open('/formalizer/logentry-form_fullpage.template.html','_blank');\">Add Log Entry</button>";
document.body.prepend(lebtnelement);
// add the darkmode button
var dmbtnelement = document.createElement("div");
dmbtnelement.innerHTML = "<button id=\"darkmode\" class=\"button button2\" onclick=\"switch_light_or_dark();\">Light / Dark</button>";
document.body.prepend(dmbtnelement);
// darkmode handling
var darkmode = 0;
function set_darkmode() {
    if (darkmode < 1) {
        document.body.removeAttribute('data-theme');
    } else {
        document.body.setAttribute('data-theme', 'dark');
    }
}
function switch_light_or_dark() {
    darkmode = 1 - darkmode;
    set_darkmode();
}
function init_darkmode() {
    set_darkmode();
}
const darkmodebutton = document.getElementById('darkmode');
function sendData( data ) {
  const XHR = new XMLHttpRequest(),
        FD  = new FormData();
  // Push our data into our FormData object
  for( name in data ) {
    FD.append( name, data[ name ] );
  }
  // Define what happens on successful data submission
  XHR.addEventListener( 'load', function( event ) {
    console.log( 'Darkmode setting stored, response loaded.' );
  } );
  // Define what happens in case of error
  XHR.addEventListener(' error', function( event ) {
    console.error( 'Failed to communicate darkmode setting.' );
  } );
  // Set up our request
  XHR.open( 'POST', '/cgi-bin/fzuistate.py' );
  // Send our FormData object; HTTP headers are set automatically
  XHR.send( FD );
}
darkmodebutton.addEventListener( 'click', function()
    { sendData( { 'darkmode' : darkmode } );
} )
// Let's read state
var uistate = new XMLHttpRequest();
uistate.onreadystatechange = function() {
    if (this.readyState == 4) {
        if (this.status == 200) {
            // Parse UI state
            console.log(uistate.response);
            var state_data = uistate.responseText;
            var json_uistate = JSON.parse(state_data);
            if (json_uistate.hasOwnProperty('darkmode')) {
                darkmode = json_uistate['darkmode'];
                set_darkmode();
            }
        } else {
            console.error('State loading failed.');
            // default init
            init_darkmode();
        }
    }
};
uistate.open( 'GET', '/cgi-bin/fzuistate.py' );
uistate.send(); // no form data presently means we're requesting state
