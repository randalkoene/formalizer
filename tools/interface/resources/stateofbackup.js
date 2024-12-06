// Use this as shown in the fzdashboard index template.html.
class backupState {

	constructor(state_id = 'backupstate', signalfile = '/formalizer/data/fzbackup-mirror-to-github.signal', autostart = true, check_interval_s = 300) {
        this.signalfile = signalfile
        this.check_interval_s = check_interval_s;
        this.intervaltask = null;
        this.state = null;
        this.stateelement = document.getElementById(state_id);
        console.log('backupState instance created');
        if (autostart) {
            this.stateStart();
        }
    }

    updateState() {
        fetch(this.signalfile)
        .then(response => response.text())
        .then(text => {
            this.state = text;
            this.stateelement.textContent = text;
        })
        .catch(error => {
            console.error('backupState error:', error);
        });
    }

    stateStart() {
        if (this.intervaltask != null) {
            console.error('Unable to start backupState when there is already an active backupState task running.');
            return;
        }
        this.updateState();
        this.intervaltask = setInterval(this.updateState.bind(this), this.check_interval_s*1000);
        console.log('backupState started');
    }

};
