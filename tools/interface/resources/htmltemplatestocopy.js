class htmlTemplates {
    /**
     * @param before_id ID of existing element before which the panel is placed.
     * @param panel_id ID that will be used for the newly created panel element.
     * @param button_text The text to write on the panel open hover button.
     * @param content_src_id The ID of the element from which content is copied into the panel.
     */
    constructor(before_id = 'after_cptplt', panel_id = 'cptplt', button_text = 'templates', content_src_id = 'templates_html') {
      this.panel_id = panel_id;
      this.cptplt = null;
      this.cptplt_closed = null;
      this.cptplt_close = null;
      this.button_element = this.makeHoverButton(before_id, button_text, content_src_id);
      console.log('HTML copy templates added.');
    }

    makeHoverButton(before_id, button_text, content_src_id) {
      var beforeelement = document.getElementById(before_id);
      var content_src = document.getElementById(content_src_id);
      var cptpltbtnelement = document.createElement("div");
      cptpltbtnelement.innerHTML = `<div class="cptplt_closed" id="cptplt_closed">${button_text}</div>\n<div class="cptplt" id="${this.panel_id}">\n${content_src.innerHTML}\n<div class="cptplt_close" id="cptplt_close">close</div>`;
      beforeelement.parentNode.insertBefore(cptpltbtnelement, beforeelement);
      this.cptplt = document.getElementById("cptplt");
      this.cptplt_closed = document.getElementById("cptplt_closed");
      this.cptplt_close = document.getElementById("cptplt_close");
      this.cptplt_closed.addEventListener(
        "mouseenter",
        (event) => {
          this.cptplt.style.display = 'block';
          this.cptplt_closed.style.display = 'none';
          this.cptplt_close.style.display = 'block';
        },
        false,
      );
      this.cptplt_close.addEventListener(
        "mouseenter",
        (event) => {
          this.cptplt.style.display = 'none';
          this.cptplt_closed.style.display = 'block';
          this.cptplt_close.style.display = 'none';
        },
        false,
      );
      return cptpltbtnelement;
    }

};

// To use this add to the HTML page something like this:
// <script type="text/javascript" src="/htmltemplatestocopy.js"></script>
// <script>const htmltemplates = new htmlTemplates('after_cptplt', 'cptplt', 'templates', 'templates_html');</script>
//
// And in the header:
// <link rel="stylesheet" href="/htmltemplatestocopy.css">
//
// You can load the copyinnerhtml.js or copyinputvalue.js scripts to easily copy
// templates contained in the template HTML to the clipboard.
//
// To specify the template HTML to add to the panel, include in the HTML page
// something like this:
// <div id="templates_html" style="display:none;">
// (the HTML content)
// </div>
