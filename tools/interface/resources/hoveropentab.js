// See how this is used in fzloghtml-cgi.py.
class HoverOpenTab {
    constructor(hoverclick_id, tab_id, start_open=true) {
        this.tab_open_ref = document.getElementById(hoverclick_id);
        this.tab_ref = document.getElementById(tab_id);
        this.tab_open = start_open;

        this.tab_open_ref.addEventListener('mouseover', () => {
            if (!this.tab_open) {
                this.tab_ref.style.display = 'block';
                this.tab_open = true;
                //console.log('tab open.');
            }
        });
        this.tab_open_ref.addEventListener('click', this.toggle_tab.bind(this));
    }

    toggle_tab() {
        if (this.tab_open) {
            this.tab_ref.style.display = 'none';
            //console.log('tab close.');
        } else {
            this.tab_ref.style.display = 'block';
            //console.log('tab open.');
        }
        this.tab_open = !this.tab_open;
    }
};
