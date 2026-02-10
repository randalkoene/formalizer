#!/usr/bin/python3
#
# metrictags.py
#
# Randal A. Koene, 20240413
#
# Generate a page on which to easily generate and copy (timestamped) metrics
# tags to include in Log entries.

try:
    import cgitb; cgitb.enable()
except:
    pass
import sys, cgi, os, stat
sys.stderr = sys.stdout
import time
from time import strftime
import traceback
from io import StringIO
from traceback import print_exc
from subprocess import Popen, PIPE

# Print this early to enable seeing errors.
print("Content-type:text/html\n\n")

tag_generators = {
    "Start Non-Music Video Viewing/Listening": ( 'NMVIDSTART', 'nmvidstart' ),
    "Stop Non-Music Video Viewing/Listening": ( 'NMVIDSTOP', 'nmvidstop' ),
}

TAG_TABLE_LINE = '''<tr><td>
<b>%s</b>:
<P>
%s <button class="button button2" onclick="copyValueToClipboard('%s');">copy</button><input id="%s" type="hidden" value="%s">
<P>
<hr>
</td></tr>
'''

TAG_GENERATOR_PAGE = '''<html>
<head>
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
<title>Metrics - Tage Generators</title>
</head>
<body>
<h1>Metrics - Tage Generators</h1>

<P>
<button class="button button1" onclick="window.open('/cgi-bin/metrictags.py','_self');">Refresh Timestamps</button>
<P>

<table><tbody>
%s
</tbody></table>

<hr>

<p>[<a href="/index.html">fz: Top</a>]</p>

<script type="module" src="/fzuistate.js"></script>
<script>
function copyValueToClipboard(hidden_id) {
  var copyValue = document.getElementById(hidden_id);
  var value_content = '---';
  if (copyValue == null) {
    console.log('Did not find object with id node_id.');
  } else {
    value_content = copyValue.value;
    console.log(`Value of hidden_id object is ${value_content}.`);
  }
  copyValue.select();
  //copyValue.setSelectionRange(0, 99999); // For mobile devices
  navigator.clipboard.writeText(value_content);
  alert("Copied: " + value_content);
}
</script>
</body>
</html>
'''

def time_stamp()->str:
    return strftime('%Y%m%d%H%M')

def make_tag_generator_page():
    timestamp = time_stamp()
    tags_table = ""
    for tag_key, tag_content in tag_generators.items():
        taglabel, hidden_id = tag_content
        tag = '@%s:%s@' % (taglabel, timestamp)
        tags_table += TAG_TABLE_LINE % ( tag_key, tag, hidden_id, hidden_id, tag )
    print(TAG_GENERATOR_PAGE % tags_table)

if __name__ == '__main__':
    # form = cgi.FieldStorage()
    # topics = form.getvalue("topics")
    # selection = form.getvalue('selection')

    make_tag_generator_page()

    sys.exit(0)
