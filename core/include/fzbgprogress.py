# Copyright 2024 Randal A. Koene
# License TBD

"""
This header file declares functions used to display the progress of 
a background process in a web page.

Versioning is based on https://semver.org/. See coreversion.hpp for more.
"""

EMBED_IN_HTML='''
<progress id="progress" value="0" max="100">0 %</progress>
'''

EMBED_IN_SCRIPT='''
function loadFile(filePath) {
  var result = null;
  var xmlhttp = new XMLHttpRequest();
  xmlhttp.open("GET", filePath, false);
  xmlhttp.send();
  if (xmlhttp.status==200) {
    result = xmlhttp.responseText;
  }
  return result;
}

const sleep = (delay) => new Promise((resolve) => setTimeout(resolve, delay));

const progressloop = async () => {
  var result=0;
  var progress = 0;
  var progressbar_ref = document.getElementById('progress');
  while (progress < 100) {
      await sleep(1000);
      var new_progress = loadFile('/formalizer/data/%s');
      if (new_progress) {
        progress = new_progress;
      }
      progressbar_ref.value = `${progress}`;
      progressbar_ref.innerHTML = `${progress} %%`;
  }
  window.location.replace("%s");
}
'''

def make_background_progress_monitor(statefile:str, donetarget:str)->tuple:
	return ( EMBED_IN_HTML, EMBED_IN_SCRIPT % (statefile, donetarget) )
