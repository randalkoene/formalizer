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

    getFromDateStamp(datestampstr) {
        const yearstr = datestampstr.substring(0,4);
        const monthstr = datestampstr.substring(4,6);
        const daystr = datestampstr.substring(6,8);
        const hourstr = datestampstr.substring(8,10);
        const minstr = datestampstr.substring(10,12);
        const secstr = datestampstr.substring(12,14);
        const dateString = `${yearstr}-${monthstr}-${daystr}T${hourstr}:${minstr}:${secstr}`; 
        return new Date(dateString);
    }

    updateState() {
        fetch(this.signalfile)
        .then(response => response.text())
        .then(text => {
            this.state = text;
            const currentdatetime = new Date();
            const signaldatetime = this.getFromDateStamp(text);
            const ms = currentdatetime.getTime() - signaldatetime.getTime();
            this.stateelement.textContent = text;
            if (ms > 86400000) {
                this.stateelement.style.color = '#ff0000';
            } else {
                this.stateelement.style.color = '#00af00';
            }
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
