// fzuistate.js - Randal A. Koene, 20210315 (updates through at least 2026)
// Formalizer Web UI standard state handling
// Note: Load this in a <script type="module"> tag within the <body> section or within $("document").ready(function(){ ... }); as when in <head> section.
//       The Javascript below ineracts with the Python CGI script fzuistate.py, which
//       processes both update and state requests. Update requests are made with valid
//       form data submitted while state requests are made by providing no form data.

// --- Buttons added to the page:

/*
A few things we need in order to extend this to allow for hover actions
displaying extra information:

- [DONE] Switch building raw buttons within container DIV to building incorporated DIVs and
  buttons within (as per test_hover_serverdata.html), perhaps only if and when a
  button is specified to have a hover action.
- [DONE] Pull the common CSS used in test_hover_serverdata.html into fzuistate.css for use
  in the hover cards.
- [DONE] Add information for bottombar_data so that it's clear which buttons have hover
  actions and what functions to use.
- [DONE] Make a section in fzuistate.js where necessary javascript is added to create
  HoverAction objects and add necessary functions.
- [DONE] Figure out best way to include async, e.g. convert all of fzuistate.js to a module,
  and then update whereever fzuistate.js is imported. (And test that it still works.)

*/

// -- Button Bars Section --

// Objects and functions needed by buttons with HoverAction
// (Adapted from test_hover_serverdata.html, see alternatives there.)

import { fzAPICall } from '/fzCGIRequest.js';
import { HoverAction } from '/hoveraction.js';

async function getSelected() {
    const data = await fzAPICall('/fz/graph/namedlists/selected.json');
    if ('selected' in data) {
        const textdata = await fzAPICall(`/fz/graph/nodes/${data['selected'][0]}/excerpt.json`);
        if ('excerpt' in textdata) {
            document.getElementById("content_logtime").textContent = `Selected [${data['selected'][0]}]: ${textdata['excerpt'].substring(0,400)}`;
        } else {
            document.getElementById("content_logtime").textContent = `Selected [${data['selected'][0]}]: (no retrieved content)`;
        }
    } else {
        document.getElementById("content_logtime").textContent = `No Selected Node`;
    }
}

async function getSuperiors() {
    const data = await fzAPICall('/fz/graph/namedlists/superiors.json');
    if ('superiors' in data) {
        document.getElementById('content_supclear').textContent = 'Superiors:';
        for (const superior of data['superiors']) {
            const textdata = await fzAPICall(`/fz/graph/nodes/${superior}/excerpt.json`);
            if ('excerpt' in textdata) {
                document.getElementById('content_supclear').textContent += ` [${superior}] ${textdata['excerpt'].substring(0,200)}`;
            } else {
                document.getElementById('content_supclear').textContent += ` [${superior}] (no retrieved content)`;
            }
        }
    } else {
        document.getElementById('content_supclear').textContent = `No selected Superiors`;
    }
}

/*
For hover action, the generated HTML produced below needs to be of a format similar
to this (from test_hover_serverdata.html):

<div style="display:inline-block;">
<div id="hover_<ID>" class="bottombar_hoverdiv">
  <div class="bottombar_carddiv">
    <span id="close_<ID>" class="bottombar_closediv">&times;</span>
    <span id="content_<ID>">Hover card content.</span>
  </div>
</div>
<button id="<ID>">Hover over this button (using HoverAction) to get selected</button>
</div>
*/

// -- Bottom bar
// To add/change buttons just modify the bottombar_data list.
// Format per list item: [ button-id, onclick-JS, button-label, hover-async-action ]
const bottombar_data = [
    [ 'logtime', "window.open('/cgi-bin/fzlogtime.cgi?source=nonlocal&cgivar=wrap','_blank');", 'Log Time', getSelected ],
    [ 'system', "window.open('/system.html', '_blank');", 'System', null ],
    [ 'calsched', "window.open('/cgi-bin/schedule-cgi.py?c=true&num_days=7&s=20','_blank');", 'Calendar Schedule', null ],
    [ 'behtools', "window.open('/formalizer/system-help.html','_blank');", 'Behavior Tools', null ],
    [ 'supclear', "window.open('/cgi-bin/fzgraph-cgi.py?action=generic&q=true&L=delete&l=superiors&E=STDOUT&W=STDOUT','_blank');", 'Clear SupNNL', getSuperiors ],
    [ 'searchgraph', "window.open('/formalizer/fzgraphsearch-form.html','_blank');", 'Search Graph', null ],
    [ 'logentry', "window.open('/formalizer/logentry-form_fullpage.template.html','_blank');", 'Add Log Entry', null ],
    [ 'addnode', "window.open('/cgi-bin/fzgraphhtml-cgi.py?edit=new','_blank');", 'Add Node', null ],
    [ 'darkmode', "switch_light_or_dark();", 'Light / Dark', null ],
];
const bottombar_div = document.createElement("div"); // container DIV for bottom buttons
bottombar_div.id = "bottombar";
document.body.prepend(bottombar_div);

var bottombar_open = true;
function bottombar_toggle() {
    const state = bottombar_open ? 'none' : 'flex';
    bottombar_div.style.display = state;
    rightbar_div.style.display = state;
    bottombar_open = !bottombar_open;
}

var hoveractions = [];
var buttontype = 2;
for (let i = 0; i < bottombar_data.length; ++i) {
    if (!bottombar_data[i][3]) {
        var btn = document.createElement("button"); // Create the button directly
        btn.id = bottombar_data[i][0];
        btn.className = `button button${buttontype} bottombar_button`;
        btn.setAttribute("onclick", bottombar_data[i][1]);
        btn.textContent = bottombar_data[i][2];
        bottombar_div.appendChild(btn); // Append the button directly to the bar
    } else {
        var btndiv = document.createElement("div"); // Create inline div for hover card and button
        btndiv.style.display = 'inline-block';

        var hoverdiv = document.createElement("div"); // Create hidden div for hover card
        hoverdiv.id = `hover_${bottombar_data[i][0]}`;
        hoverdiv.className = 'bottombar_hoverdiv';

        var carddiv = document.createElement("div"); // Create card div for hover card
        carddiv.className = 'bottombar_carddiv';

        var contentspan = document.createElement("span"); // Create span for hover card content
        contentspan.id = `content_${bottombar_data[i][0]}`;

        var btn = document.createElement("button"); // Create button
        btn.id = bottombar_data[i][0];
        btn.className = `button button${buttontype} bottombar_button`;
        btn.setAttribute("onclick", bottombar_data[i][1]);
        btn.textContent = bottombar_data[i][2];

        carddiv.appendChild(contentspan);
        hoverdiv.appendChild(carddiv);
        btndiv.appendChild(hoverdiv);
        btndiv.appendChild(btn);
        bottombar_div.appendChild(btndiv);

        //hoverdiv.style.bottom = `${btn.offsetHeight}px`;

        hoveractions.push( new HoverAction(btn.id, hoverdiv.id, true, null, null, bottombar_data[i][3]) );
    }
    // Logic for button types
    buttontype -= 1;
    if (buttontype == 0) buttontype = 2;
}

// -- Right bar
const rightbar_data = [
    [ 'fztop', "window.open('/','_blank');", 'Top' ],
];
const rightbar_div = document.createElement("div");
rightbar_div.id = "rightbar";
document.body.prepend(rightbar_div);

buttontype = 2;
for (let i = 0; i < rightbar_data.length; ++i) {
    var rbtn = document.createElement("button");
    rbtn.id = rightbar_data[i][0];
    rbtn.className = `button button${buttontype}`;
    rbtn.setAttribute("onclick", rightbar_data[i][1]);
    rbtn.textContent = rightbar_data[i][2];
    
    buttontype -= 1;
    if (buttontype == 0) buttontype = 2;

    rightbar_div.appendChild(rbtn);
}

// -- Bars toggle
const bottombar_togglediv = document.createElement("div");
bottombar_togglediv.innerHTML = `<button id="bottombar-toggle" class="button button3" onclick="bottombar_toggle();">::</button>`;
bottombar_togglediv.style.position = "fixed";
bottombar_togglediv.style.bottom = "0";
bottombar_togglediv.style.right = "0";
bottombar_togglediv.style.zIndex = "1001"; // Keep it above the bar
document.body.prepend(bottombar_togglediv);

// -- UI State Section --

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

function set_noncolor_theme_details(json_uistate) {
    if (json_uistate.hasOwnProperty('buttonradius')) {
        document.documentElement.style.setProperty('--btn-radius', `${json_uistate['buttonradius']}px`);
    }
}

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
// *** There's an alternative way to load/store this data using JS localStorage,
//     but perhaps that would be harder for other parts of the Formalizer to
//     access and modify, as is expected here with fzuistate.py.
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
            set_noncolor_theme_details(json_uistate);
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
