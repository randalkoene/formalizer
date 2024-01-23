/**
 * fztask.js, Randal A. Koene 2024
 *
 * Runs a timer bar based on Log Chunk opening events.
 *
 * To include this on a page:
 * - Load this before the closing </body> tag: <script type="text/javascript" src="/fztask.js">v</script>
 * - Load the necessary CSS in the <head> section (e.g. before <title>): <link rel="stylesheet" href="/fztask.css">
 * - Add a container for the timer bar, somewhere in the body:
 *   <div class="timerBar">
 *       <div class="timerBarFill">
 *       </div>
 *       <div class="timerBarTextContainer">
 *           <div class="timerBarText"></div>
 *       </div>
 *   </div>
 * - In something like the onload function, create a timer bar object, e.g.:
 *   window.onload = function() {
 *     var timer_bar = new TimerBar();
 *   }
 * - Create an instance of floatClock within a <script> section or a Javascript component, e.g: <script>var clock = new floatClock('clock');</script>
 */

const fill = document.querySelector(".timerBarFill");
const text = document.querySelector(".timerBarText");
let chunk_seconds = 20*60;
// a bit pointless, because CSS pixels are not real pixels, better to just redraw each second
//let secondsperupdate = chunk_seconds / 600; // e.g. 20*60 / 600 = 2 seconds
let secondsperupdate = 1;
let secondsLeft = chunk_seconds;
let heightPct = 100;

// every hundred millisecond, but probably should only do when a line less
//let countDown = setInterval(tick, secondsperupdate * 1000);
let countDown = null;

function tick() {
    secondsLeft -= secondsperupdate;
    var minsleft = Math.floor(secondsLeft / 60);
    var secsleft = secondsLeft % 60;
    var secstr = ("0" + secsleft).substr(-2);
    text.textContent = `${minsleft}:${secstr}`;
    if (secondsLeft === 0) {
        clearInterval(countDown);
        fill.style.height = 0;
        return;
    }
    heightPct = 100 * secondsLeft / chunk_seconds;
    //console.log(`${heightPct}%`);
    fill.style.height = `${heightPct}%`;
    if (heightPct < 50) {
        fill.style.backgroundColor = '#f2ed6d';
    }
    if (heightPct < 25) {
        fill.style.backgroundColor = '#ea7267';
    }

}

class TimerBar {
	constructor(active_timerbar = true) {
		this.taskserveraddr = 'http://localhost:5000';
		this.source = null;
        console.log('TimerBar created.');
        if (active_timerbar) {
            this.init();
        }
    }

    init() {
    	// subscribe to Task Chunk messages
	    this.source = new EventSource(this.taskserveraddr+'/listen');

	    // handle messages
	    this.source.onmessage = function(event) {
	        // Do something with the data:
	        console.log(event.data);
	    };

	    this.source.addEventListener("TC_start", function(event) {
	        const t = JSON.parse(event.data).t
	        const mins = JSON.parse(event.data).mins
	        console.log('TC_start t = '+ t + ', mins = ' + mins)
	        chunk_seconds = mins * 60;
	        secondsLeft = chunk_seconds;
	        countDown = setInterval(tick, secondsperupdate * 1000);
	    });

	    this.source.addEventListener("TC_end", function(event) {
	        const t = JSON.parse(event.data).t
	        console.log('TC_end t = '+ t)
	        clearInterval(countDown);
	        secondsLeft = 0;
	    });

	    this.source.addEventListener("TC_state", function(event) {
	        const active = JSON.parse(event.data).active
	        console.log('TC_state state = ' + active)
	        const t = JSON.parse(event.data).t
	        if (active) {
	            const mins = JSON.parse(event.data).mins
	            console.log('TC_state ACTIVE t_open = '+ t + ', mins = ' + mins)
	            chunk_seconds = mins * 60;
	            var jststamp = t.substring(0,3) + '-' + t.substring(4,5) + '-' + t.substring(6,7) + 'T' + t.substring(8,9) + ':' + t.substring(10,11) + ':00';
	            var jschunk_start = new Date(jststamp);
	            var jstnow = new Date();
	            var secs_tdiff = (jstnow - jschunk_start) / 1000;
	            if (secs_tdiff >= chunk_seconds) {
	                secondsLeft = 0;
	            } else {
	                secondsLeft = chunk_seconds - secs_tdiff;
	            }
	            countDown = setInterval(tick, secondsperupdate * 1000);
	        } else {
	            console.log('TC_state INACTIVE t_close = '+ t)
	            clearInterval(countDown);
	            secondsLeft = 0;
	        }
	    });

	    // Request the initial Task state.
    	this.request_TCstate();
    }

    request_TCstate() {
        const XHR = new XMLHttpRequest(),
        FD  = new FormData();
        // This needs no actual FormData
        // Define what happens on successful data submission
        XHR.addEventListener( 'load', function( event ) {
            console.log( 'TC status request sent, response loaded.' );
        } );
        // Define what happens in case of error
        XHR.addEventListener(' error', function( event ) {
            console.error( 'Failed to communicate TC status request.' );
        } );
        // Set up our request
        XHR.open( 'GET', this.taskserveraddr + '/state' ); // was POST
        // Send our FormData object; HTTP headers are set automatically
        XHR.send(); // was ( FD )
    }

};

window.onload = function() {
    var timer_bar = new TimerBar(false); // set to true to connect with task server
};
// Could add connection stop detection as in https://medium.com/conectric-networks/a-look-at-server-sent-events-54a77f8d6ff7
