// To use this, include something like:
// <script type="text/javascript" src="/exportpage.js"></script>
// <script>
// const export_page = ExportPage();
// </script>
// <button onclick="export_page.prepareExport();">Export Page</button>

class ExportPage {
  constructor() {
    this.filename = '';
    this.exportname_div = null;
    this.exportname_input = null;
  }

  // Open an input box for the filename.
  ExportHTML() {
    this.makeFileNameBox();
    this.exportname_input.addEventListener('change', this.startExportHTML.bind(this));
  }

  ExportSVG() {
    this.makeFileNameBox();
    this.exportname_input.addEventListener('change', this.startExportSVG.bind(this));
  }

  makeFileNameBox() {
    this.exportname_div = document.createElement('div');
    this.exportname_div.innerHTML = `Export file name:<br><input id="exportname" type="text"><button id="exportcancel">cancel</button>`;
    this.exportname_div.style.position = 'fixed';
    this.exportname_div.style.top = '50%';
    this.exportname_div.style.left = '50%';
    this.exportname_div.style.transform = 'translate(-50%, -50%)';
    this.exportname_div.style.backgroundColor = 'var(--bgcolor-textarea)';
    this.exportname_div.style.textAlign = 'center';
    this.exportname_div.style.justifyContent = 'center';
    document.body.appendChild(this.exportname_div);
    this.exportname_input = document.getElementById('exportname');
    document.getElementById('exportcancel').onclick = this.cancelExportHTML.bind(this);
  }

  cancelExportHTML() {
    this.exportname_div.remove();
  }

  startExportHTML() {
    // Get the export file name.
    this.filename = this.exportname_input.value;
    // Close the input box.
    this.exportname_div.remove();
    // Export.
    this.exportToHTML();
  }

  exportToHTML() {
    const html = document.documentElement.outerHTML;
    const blob = new Blob([html], { type: 'text/html' });
    const url = URL.createObjectURL(blob);

    const a = document.createElement('a');
    a.href = url;
    a.download = this.filename;
    a.click();
    URL.revokeObjectURL(url);
  }

  startExportSVG() {
    // Get the export file name.
    this.filename = this.exportname_input.value;
    // Close the input box.
    this.exportname_div.remove();
    // Export.
    // This is handled outside of this class, see the example in Options_pane.template.html.
  }

  ExportScreenshot() {
    window.URL = window.URL || window.webkitURL;
    window.open(window.URL.createObjectURL(this.exportToScreenshot()));
  }

  exportToScreenshot() {
    var screenshot = document.documentElement
    .cloneNode(true);
    screenshot.style.pointerEvents = 'none';
    screenshot.style.overflow = 'hidden';
    screenshot.style.webkitUserSelect = 'none';
    screenshot.style.mozUserSelect = 'none';
    screenshot.style.msUserSelect = 'none';
    screenshot.style.oUserSelect = 'none';
    screenshot.style.userSelect = 'none';
    screenshot.dataset.scrollX = window.scrollX;
    screenshot.dataset.scrollY = window.scrollY;
    var blob = new Blob([screenshot.outerHTML], {
      type: 'text/html'
    });
    return blob;
  }
};

// function flattenHTML() {
//   let newHTML = '<html><head>';
//   // Collect CSS styles
//   for (let sheet of document.styleSheets) {
//     newHTML += `<style>${sheet.cssText}</style>`;
//   }
//   // Collect JavaScript files
//   let scriptTags = document.querySelectorAll('script');
//   for (let script of scriptTags) {
//     newHTML += `<script src="${script.src}"></script>`;
//   }
//   newHTML += '</head><body>' + document.body.innerHTML + '</body></html>';
//   // Optional: Replace the current document with the flattened HTML
//   document.documentElement.innerHTML = newHTML;
// }
