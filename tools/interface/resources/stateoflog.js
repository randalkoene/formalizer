// This sets the variable stateoflog to a list such as [true, 202410021403].
function isInteger(value) {
    return /^-?\d+$/.test(value);
}
var stateoflog = [];
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
            		if isInteger(words[0]) {
            			timestamp = words[0];
            		}
            		isopen = (words[1] == 'OPEN');
            		stateoflog = [isopen, timestamp];
            	}
            }
        } else {
            console.error('Log state loading failed.');
        }
    }
};
logstate.open( 'GET', '/cgi-bin/fzloghtml-cgi.py&mostrecentraw=on' );
logstate.send(); // no form data presently means we're requesting state
