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

    updateClock() {
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

        // Show updated time
        if (this.showseconds) {
            this.clockelement.innerHTML = this.currentHours + ":" + this.currentMinutes + ":" + this.currentSeconds;
        } else {
            this.clockelement.innerHTML = this.currentHours + ":" + this.currentMinutes;
        }
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
