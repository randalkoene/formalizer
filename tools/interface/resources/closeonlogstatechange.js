// This uses stateoflog.js to a) obtain the initial Log state open creating a page,
// b) regularly check the state of Log, and c) close the page if the state changed.

// import { logState } from 'stateoflog.js';

class closeOnLogStateChange {

	constructor(logstate_id = 'logstate', autostart = true, check_interval_s = 3) {
        this.initial_state = null;
        this.stateoflog_obj = new logState(
        	logstate_id,
        	autostart,
        	false, // make_state_element
        	check_interval_s,
        	this.checkCloseOnChange.bind(this) );
        console.log('closeOnLogStateChange instance created');
    }

    checkCloseOnChange(stateoflog_obj) {
    	if (this.initial_state != null) {
    		// Check if Log state changed.
    		if (stateoflog_obj.stateoflog[1] != this.initial_state) {
    			// Close page.
    			window.close();
    		}

    	} else {
    		// Set start at page build.
    		this.initial_state = stateoflog_obj.stateoflog[1];
    	}
    }

};
