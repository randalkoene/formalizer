// See how the classes below are used in fztask-cgi.py.
// Randal A. Koene, 20241022

class getNNL {
  constructor(list_name, idx=null) {
    this.list_name = list_name;
    this.idx = idx;
    this.received_NNL = '';
    this.NNL_content = null;
    this.request_done = false;
    fetch(`/cgi-bin/fzgraph-cgi.py?action=generic&C=/fz/graph/namedlists/${list_name}.json&o=STDOUT&q=true&minimal=true`, {
      mode: 'no-cors'
    })
    .then(response => response.text())
    .then(data => {
      //console.log(`NNL content response: ${data}`);
      var emptyline_idx = data.indexOf('\n\n');
      var from_idx = emptyline_idx + 2;
      if (emptyline_idx == -1) {
        emptyline_idx = data.indexOf('\r\n\r\n');
        from_idx = emptyline_idx + 4;
      }
      if (emptyline_idx >= 0) {
        try {
          const data_json = JSON.parse(data.substring(from_idx));
          const data_keys = Object.keys(data_json);
          this.received_NNL = data_keys[0];
          if (this.idx == null) {
            this.NNL_content = data_json[this.received_NNL];
          } else {
            this.NNL_content = data_json[this.received_NNL][this.idx];
          }
        } catch {
          console.error('Unable to parse JSON NNL data.');
        }
      } else {
        console.error('No data found.');
      }
      this.request_done = true;
      
    });
  }
};

class compareSelected {
  constructor(compare_with, do_when_different, interval_ms=1000) {
    this.compare_with = compare_with;
    this.do_when_different = do_when_different;
    this.selected = null;
    setInterval(this.checkSelected.bind(this), interval_ms);
  };

  checkSelected() {
    if (this.selected) { // A call was made.
      if (this.selected.request_done) {
        // Process request result.
        if (this.selected.NNL_content) {
          // Compare.
          if (this.selected.NNL_content != this.compare_with) this.do_when_different(this.selected.NNL_content);
        }
        // Make new request.
        this.selected = new getNNL('selected', 0);
      }
    } else { // No call was made yet.
      this.selected = new getNNL('selected', 0);
    }
  }
};
