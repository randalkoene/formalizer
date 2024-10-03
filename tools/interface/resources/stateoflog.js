// This sets the variable stateoflog to a list such as [true, 202410021403].
var stateoflog_elementref = null;
var stateoflog = [];

function parse_and_show_stateoflog(is_open, timestampstr) {
	if (stateoflog_elementref) {
		const year = timestampstr.substring(0,4);
		const month = timestampstr.substring(4,6);
		const day = timestampstr.substring(6,8);
		const hour = timestampstr.substring(8,10);
		const minute = timestampstr.substring(10,12);
		const date = new Date(`${year}-${month}-${day}T${hour}:${minute}:00`);
		const currentdate = new Date();
		console.log(`Open stamp: ${date}, current stamp: ${currentdate}`)
		const diffInMilliseconds = Math.abs(date - currentdate);
		console.log(`Milliseconds: ${diffInMilliseconds}`)
		const minutes = Math.floor(diffInMilliseconds / (1000 * 60));
		console.log(`Minutes: ${minutes}`)
		if (is_open) {
			stateoflog_elementref.innerHTML = `${minutes}`;
		} else {
			stateoflog_elementref.innerHTML = 'closed';
		}
	}
}

function isInteger(value) {
    return /^-?\d+$/.test(value);
}

var logstate = new XMLHttpRequest();
logstate.onreadystatechange = function() {
    if (this.readyState == 4) {
        if (this.status == 200) {
            // Parse UI state
            console.log(`fzloghtml-cgi.py response: ${logstate.response}`);
            var state_data = logstate.responseText;
            const lines = state_data.split('\n');
            var timestamp = '';
            var isopen = false;
            for (let i = 0; i < lines.length; ++i) {
            	const words = lines[i].split(' ');
            	if (words.length > 1) {
            		if (isInteger(words[0])) {
            			timestamp = words[0];
            		}
            		isopen = (words[1] == 'OPEN');
            		stateoflog = [isopen, timestamp];
            		parse_and_show_stateoflog(isopen, timestamp);
            	}
            }
        } else {
            console.error('Log state loading failed.');
        }
    }
};

class logState {

	constructor(logstate_id = 'logstate', autostart = true) {
        this.intervaltask = null;
        this.stateoflog = [];
        this.stateoflogelement = this.getStateOfLogElement(logstate_id);
        stateoflog_elementref = this.stateoflogelement;
        console.log('logState instance created');
        if (autostart) {
            this.logstateStart();
        }
    }

    getStateOfLogElement(logstate_id) {
        var stateoflogelement = document.getElementById(logstate_id);
        if (stateoflogelement == null) {
            console.log('Making stateoflog element');
            stateoflogelement = this.makeStateOfLogElement(logstate_id);
        }
        return stateoflogelement;
    }

    makeStateOfLogElement(logstate_id) {
        var stateoflogelement = document.createElement("div");
        g.setAttribute("id", logstate_id);
        document.body.prepend(stateoflogelement);
        return document.getElementById(logstate_id);
    }

    updateStateOfLog() {
    	logstate.open( 'GET', '/cgi-bin/fzloghtml-cgi.py?mostrecentraw=on' );
		logstate.send(); // no form data presently means we're requesting state

		// Note that this update of the object state may be one interval behind,
		// because the call via logstate is asynchronous.
		if (stateoflog.length >= 2) {
			this.stateoflog = stateoflog;
		}
    }

    logstateStart() {
        if (this.intervaltask != null) {
            console.error('Unable to start logState when there is already an active logState task running.');
            return;
        }
        this.updateStateOfLog();
        this.intervaltask = setInterval(this.updateStateOfLog.bind(this), 60000); // every minute
        console.log('logState started');
    }

};
