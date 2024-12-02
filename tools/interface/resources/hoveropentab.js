// See how this is used in fzloghtml-cgi.py.
class HoverOpenTab {
    constructor(hoverclick_id, tab_id, start_open=true, apply_id='', apply_func=null, open_func=null) {
        this.tab_open_ref = document.getElementById(hoverclick_id);
        this.tab_ref = document.getElementById(tab_id);
        this.tab_open = start_open;
        this.apply_ref = null;
        this.apply_func = apply_func;
        this.open_func = open_func;

        this.tab_open_ref.addEventListener('mouseover', () => {
            if (!this.tab_open) {
                this.tab_ref.style.display = 'block';
                this.tab_open = true;
                //console.log('tab open.');
                if (this.open_func) {
                    this.open_func();
                }
            }
        });
        if (apply_id=='') {
            this.tab_open_ref.addEventListener('click', this.toggle_tab.bind(this));
        } else {
            this.apply_ref = document.getElementById(apply_id);
            this.apply_ref.addEventListener('click', this.toggle_tab.bind(this));
        }
    }

    toggle_tab() {
        if (this.tab_open) {
            this.tab_ref.style.display = 'none';
            //console.log('tab close.');
            if (this.apply_func) {
                this.apply_func();
            }
        } else {
            this.tab_ref.style.display = 'block';
            //console.log('tab open.');
        }
        this.tab_open = !this.tab_open;
    }
};
