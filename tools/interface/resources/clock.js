/**
 * clock.js, Randal A. Koene 2021
 *
 * Floats a digital clock in the upper right hand corner of the browser view port.
 *
 * To include this on a page:
 * - Load this before the closing </body> tag: <script type="text/javascript" src="/clock.js">v</script>
 * - Load the necessary CSS in the <head> section (e.g. before <title>): <link rel="stylesheet" href="/clock.css">
 * - Add a button to associate the clock with, somewhere in the body: <button id="clock" class="button button2">_____</button>
 * - Create an instance of floatClock within a <script> section or a Javascript component, e.g: <script>var clock = new floatClock('clock');</script>
 *
 * Clock script adapted from https://codepen.io/kase/pen/LxNzWj.
 */

class floatClock {
    /**
     * @param clock_id The DOM `id` of the element that displays the clock.
     * @param showseconds If true then show 'HH:MM:SS'.
     * @param autostart If true then call `this.clockStart()` in object constructor.
     */
    constructor(clock_id = 'clock', showseconds = false, autostart = true) {
        this.showseconds = showseconds;
        this.intervaltask = null;
        this.currentHours = '??';
        this.currentMinutes = '??';
        this.currentSeconds = '??';
        this.clockelement = document.getElementById(clock_id);
        console.log('floatClock instance created');
        if (autostart) {
            this.clockStart();
        }
    }

    makeClockString( hrs, mins, secs ) {
        if (this.showseconds) {
            return hrs + ":" + mins + ":" + secs;
        } else {
            return hrs + ":" + mins;
        }
    }

    updateClock() {
        var clockmode = 0;
        var tz_offset_hours = 9;
        if (typeof window.global_clockmode !== 'undefined') {
            clockmode = window.global_clockmode;
        }
        if (typeof window.global_timezone_offset_hours !== 'undefined') {
            tz_offset_hours = parseInt(window.global_timezone_offset_hours);
            //console.log(`tz_offset_hours = ${tz_offset_hours}`);
        }
        var currentTime = new Date();
        // Operating System Clock 24-hour Hours, Minutes, and (optionally) Seconds
        this.currentHours = currentTime.getHours();
        this.currentMinutes = currentTime.getMinutes();
        if (this.showseconds) {
            this.currentSeconds = currentTime.getSeconds();
        }

        // Prepend '0' as needed
        this.currentMinutes = (this.currentMinutes < 10 ? "0" : "") + this.currentMinutes;
        if (this.showseconds) {
            this.currentSeconds = (this.currentSeconds < 10 ? "0" : "") + this.currentSeconds;
        }

        var mainclockstr = this.makeClockString(this.currentHours, this.currentMinutes, this.currentSeconds);
        var clockstr = mainclockstr;

        // Other time zone(s)
        if (clockmode > 0) {
            var tzhours = (this.currentHours + tz_offset_hours) % 24;
            var tzclockstr = this.makeClockString(tzhours, this.currentMinutes, this.currentSeconds);
            clockstr += "|"+tzclockstr;
        }

        // Show updated time
        this.clockelement.innerHTML = clockstr;
    }

    clockStart() {
        if (this.intervaltask != null) {
            console.error('Unable to start clock when there is already an active clock interval task running.');
            return;
        }
        this.updateClock();
        if (this.showseconds) {
            this.intervaltask = setInterval(this.updateClock.bind(this), 1000); // every second
        } else {
            this.intervaltask = setInterval(this.updateClock.bind(this), 60000); // every minute
        }
        console.log('floatClock started');
    }
};

window.global_clock = new floatClock('clock');
