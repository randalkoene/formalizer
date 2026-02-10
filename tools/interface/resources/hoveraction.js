/*
  Create a pop-up when hovering over an element and optionally
  carry out specific actions when opening and closing, as well
  as optionally using specified pop-up opening/closing functions.

  E.g., see how this is used in fzuistate.js and in
        test_hover_serverdata.html.

  Randal A. Koene, 20260210
*/

class HoverAction {
  constructor(hoverclick_id, tab_id, close_if_mouseout=true, open_func=null, close_func=null, async_action=null, handler=null) {
      this.tab_open_ref = document.getElementById(hoverclick_id);
      this.tab_ref = document.getElementById(tab_id);
      this.open_func = open_func;
      this.close_func = close_func;
      this.async_action = async_action;
      this.handler = handler;
      this.action_busy = false;

      this.tab_open_ref.addEventListener('mouseover', async (event) => {
          this.open_tab(this.async_action, this.handler);
      });
      if (close_if_mouseout) {
        this.tab_open_ref.addEventListener('mouseout', () => {
            this.close_tab();
        });
      }
  }

  default_open() {
    this.tab_ref.style.display = 'block';
  }

  default_close() {
    this.tab_ref.style.display = 'none';
  }

  async open_tab(async_action, handler) {
    if (this.tab_ref.style.display == 'none') {
      if (this.open_func) {
        this.open_func();
      } else {
        this.default_open();
      }
      if (async_action && (!this.action_busy)) {
        this.action_busy = true;
        const response = await async_action();
        if (handler) {
          handler(response);
        }
        this.action_busy = false;
      }
    }
  }

  close_tab() {
    if (this.tab_ref.style.display != 'none') {
      if (this.close_func) {
        this.close_func();
      } else {
        this.default_close();
      }
    }
  }

};

export { HoverAction };
