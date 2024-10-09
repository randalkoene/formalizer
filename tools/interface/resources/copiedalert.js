class copiedAlert {

	constructor() {
		this.alert_is_open = false;
		this.alert_counter = 0;
		this.copied_alert_HTML = '';
		this.alertpane = document.createElement("div");
		this.alertpane.classList.add('copiedAlert');
        document.body.prepend(this.alertpane);
	}

	showAlert(copied_alert_HTML) {
		this.copied_alert_HTML = copied_alert_HTML;
		this.alert_counter += 1;
		this.alertpane.innerHTML = 'Copied:<br>'+this.copied_alert_HTML;
		this.alertpane.style.display = 'block';
		this.alert_is_open = true;
		setTimeout(this.closeAlert.bind(this, this.alert_counter), 10000); // after 10 seconds
	}

	closeAlert(alert_idx) {
		if (this.alert_is_open && (this.alert_counter == alert_idx)) {
			this.alert_is_open = false;
			this.alertpane.innerHTML = '';
			this.alertpane.style.display = 'none';
		}
	}

};
const copiedAlert_ref = new copiedAlert();
