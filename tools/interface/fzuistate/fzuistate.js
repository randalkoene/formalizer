// fzuistate.js - Randal A. Koene, 20210315
// Formalizer Web UI standard state handling
// Note: Load this in a <script> tag at end of <body> section or within $("document").ready(function(){ ... }); as when in <head> section.
//       The Javascript below ineracts with the Python CGI script fzuistate.py, which
//       processes both update and state requests. Update requests are made with valid
//       form data submitted while state requests are made by providing no form data.

// --- Buttons added to the page:

const bottombar_data = [
    [ 'system', "window.open('/system.html', '_blank');", 'System' ],
    [ 'logtime', "window.open('/cgi-bin/fzlogtime.cgi?source=nonlocal&cgivar=wrap','_blank');", 'Log Time' ],
    [ 'calsched', "window.open('/cgi-bin/schedule-cgi.py?c=true&num_days=7&s=20','_blank');", 'Calendar Schedule' ],
    [ 'behtools', "window.open('/formalizer/system-help.html','_blank');", 'Behavior Tools' ],
    [ 'supclear', "window.open('/cgi-bin/fzgraph-cgi.py?action=generic&q=true&L=delete&l=superiors&E=STDOUT&W=STDOUT','_blank');", 'Clear SupNNL' ],
    [ 'logentry', "window.open('/formalizer/logentry-form_fullpage.template.html','_blank');", 'Add Log Entry' ],
    [ 'addnode', "window.open('/cgi-bin/fzgraphhtml-cgi.py?edit=new','_blank');", 'Add Node' ],
    [ 'darkmode', "switch_light_or_dark();", 'Light / Dark' ],
    [ 'fztop', "window.open('/','_blank');", 'Top' ],
];
const bottombar_div = document.createElement("div");
document.body.prepend(bottombar_div);

var bottombar_open = true;
function bottombar_toggle() {
    if (bottombar_open) {
        bottombar_div.style.display = 'none';
    } else {
        bottombar_div.style.display = 'block';
    }
    bottombar_open = !bottombar_open;
}

var buttontype = 2;
for (let i = 0; i < bottombar_data.length; ++i) {
    var btnelement = document.createElement("div");
    btnelement.innerHTML = `<button id="${bottombar_data[i][0]}" class="button button${buttontype}" onclick="${bottombar_data[i][1]}">${bottombar_data[i][2]}</button>`;
    buttontype -= 1;
    if (buttontype == 0) {
        buttontype = 2;
    }
    bottombar_div.appendChild(btnelement);
}
const bottombar_togglediv = document.createElement("div");
bottombar_togglediv.innerHTML = `<button id="bottombar" class="button button3" onclick="bottombar_toggle();">::</button>`;
document.body.prepend(bottombar_togglediv);

// Request server-side storage of data:
function sendData( data ) {
  const XHR = new XMLHttpRequest(),
        FD  = new FormData();
  // Push our data into our FormData object
  for( name in data ) {
    FD.append( name, data[ name ] );
  }
  // Define what happens on successful data submission
  XHR.addEventListener( 'load', function( event ) {
    console.log( 'Darkmode and clock settings stored, response loaded.' );
  } );
  // Define what happens in case of error
  XHR.addEventListener(' error', function( event ) {
    console.error( 'Failed to communicate darkmode and clock settings.' );
  } );
  // Set up our request
  XHR.open( 'POST', '/cgi-bin/fzuistate.py' );
  // Send our FormData object; HTTP headers are set automatically
  XHR.send( FD );
}

// --- Page color choice:
// darkmode handling
var nummodes = 5;
var darkmode = 0;
function set_darkmode() {
    if (darkmode < 1) {
        //document.body.removeAttribute('data-theme');
        document.body.setAttribute('data-theme', 'white');
    } else if (darkmode == 1) {
        document.body.setAttribute('data-theme', 'lightblue');
    } else if (darkmode == 2) {
        document.body.setAttribute('data-theme', 'darkblue1');
    } else if (darkmode == 3) {
        document.body.setAttribute('data-theme', 'darkblue2');
    } else {
        document.body.setAttribute('data-theme', 'black');
    }
}
function store_data() {
    var data = { 'darkmode' : darkmode, 'clockmode': clockmode, 'tz_offset_hours': timezone_offset_hours };
    console.log(`Sending to fzuistate.py: ${JSON.stringify(data)}`);
    sendData( data );
}
function switch_light_or_dark() {
    darkmode += 1;
    if (darkmode >= nummodes) {
        darkmode = 0;
    }
    set_darkmode();
    store_data();
}
function init_darkmode() {
    set_darkmode();
}
const darkmodebutton = document.getElementById('darkmode');

// --- Clock mode choice:
var numclockmodes = 2;
var clockmode = 0;
var timezone_offset_hours = 9;
function set_clockmode() {
    window.global_clockmode = clockmode;
    if (typeof window.global_clock !== 'undefined') {
        window.global_clock.updateClock();
    }
}
function update_clock_visible_and_stored() {
    set_clockmode();
    store_data();
}
function switch_single_multiple() {
    clockmode += 1;
    if (clockmode >= numclockmodes) {
        clockmode = 0;
        console.log('Switched to single time zone mode.');
    } else {
        console.log('Switched to multiple time zone mode.');
    }
    update_clock_visible_and_stored();
}
function init_clockmode() {
    set_clockmode();
}
var timezone_input = null;
function hide_timezone_input() {
    if (timezone_input) {
        document.body.removeChild(timezone_input);
        timezone_input = null;
    }
}
function update_timezone_offset_global_and_local(hours) {
    timezone_offset_hours = hours;
    window.global_timezone_offset_hours = hours;
}
function update_timezone_offset(hours) {
    update_timezone_offset_global_and_local(hours);
    update_clock_visible_and_stored();
}
function show_timezone_input() {
    console.log('Showing time zone input.');
    timezone_input = document.createElement('div');
    timezone_input.style.cssText = 'position:absolute;top:0;left:0;width:100%;height:100%;z-index:100;background:rgba(30,30,30,0.5)';
    document.body.appendChild(timezone_input);
    timezone_input.innerHTML = `<div style="position:absolute;top:45%;left:45%;z-index:101;background:#fff">`
        +`<div id="tz_close" style="color:red;display:inline-block;cursor:pointer;">X</div>`
        +` Time zone offset hours: <input style="display:inline-block;" type="number" id="tzhours" name="tzhours" value=${timezone_offset_hours}></div>`;
    var tzhours_ref = document.getElementById('tzhours');
    var tzclose_ref = document.getElementById('tz_close');
    tzhours_ref.addEventListener( 'change', function(e)
        {
            update_timezone_offset(parseInt(e.target.value));
            hide_timezone_input();
        }
    );
    tzclose_ref.addEventListener( 'click', function()
        {
            hide_timezone_input();
        }
    );
}
const clockmodebutton = document.getElementById('clock');

// In addition to running the function that is already attached to the respective
// buttons, also call sendData().
// if (darkmodebutton) {
//     console.log('Adding click event handler for darkmode button.');
//     darkmodebutton.addEventListener( 'click', function()
//         {
//             sendData( { 'darkmode' : darkmode, 'clockmode': clockmode } );
//         }
//     );
// }
if (clockmodebutton) {
    console.log('Adding click event handlers for clock button.');
    clockmodebutton.addEventListener( 'click', function()
        {
            switch_single_multiple();
        }
    );
    clockmodebutton.addEventListener( 'contextmenu', function(e)
        {
            show_timezone_input();
            e.preventDefault();
        }
    );
}

// Let's read state
var uistate = new XMLHttpRequest();
uistate.onreadystatechange = function() {
    if (this.readyState == 4) {
        if (this.status == 200) {
            // Parse UI state
            console.log(`fzuistate.py response: ${uistate.response}`);
            var state_data = uistate.responseText;
            var json_uistate = JSON.parse(state_data);
            if (json_uistate.hasOwnProperty('darkmode')) {
                darkmode = Number(json_uistate['darkmode']);
                set_darkmode();
            }
            if (json_uistate.hasOwnProperty('clockmode')) {
                clockmode = Number(json_uistate['clockmode']);
                set_clockmode();
            }
            if (json_uistate.hasOwnProperty('tz_offset_hours')) {
                update_timezone_offset_global_and_local(Number(json_uistate['tz_offset_hours']));
                set_clockmode();
            }
        } else {
            console.error('State loading failed.');
            // default init
            init_darkmode();
            init_clockmode();
        }
    }
};
uistate.open( 'GET', '/cgi-bin/fzuistate.py' );
uistate.send(); // no form data presently means we're requesting state
