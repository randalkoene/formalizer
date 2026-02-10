#!/usr/bin/python3
#
# metrictags.py
#
# Randal A. Koene, 20240416
#
# Generate a page on which to easily generate and copy template HTML
# to use in a Log entry.
#
# Note that the method used here is copied from metrictags.py, while
# the sorts of templates provided are similar to those provided by
# logentry-form.py.

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
    "Check List": ( 'checklist', '''<ul>
<li><input type="checkbox" > </li>
<li><input type="checkbox" > </li>
<li><input type="checkbox" > </li>
</ul>
'''),
}

TAG_TABLE_LINE = '''<tr><td>
<h3>%s</h3>
<div id="%s">
%s
</div><button class="button button2" onclick="copyHtmlToClipboard('%s');">copy</button>
<P>
<hr>
</td></tr>
'''

TAG_GENERATOR_PAGE = '''<html>
<head>
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
<title>Log Copy Templates</title>
</head>
<body>
<h1>Log Copy Templates</h1>

<table><tbody>
%s
</tbody></table>

<hr>

<p>[<a href="/index.html">fz: Top</a>]</p>

<script type="module" src="/fzuistate.js"></script>
<script>
function copyHtmlToClipboard(template_id) {
  var copyRef = document.getElementById(template_id)
  const el = document.createElement("textarea");
  var copyValue = copyRef.innerHTML;
  el.value = copyValue
  document.body.appendChild(el);
  el.select();
  navigator.clipboard.writeText(el.value);
  //document.execCommand("copy");
  document.body.removeChild(el);
  alert("Copied: " + copyValue);
}
</script>
</body>
</html>
'''

def time_stamp()->str:
    return strftime('%Y%m%d%H%M')

def make_tag_generator_page():
    tags_table = ""
    for tag_key, tag_content in tag_generators.items():
        taglabel, content = tag_content
        tags_table += TAG_TABLE_LINE % ( tag_key, taglabel, content, taglabel )
    print(TAG_GENERATOR_PAGE % tags_table)

if __name__ == '__main__':
    # form = cgi.FieldStorage()
    # topics = form.getvalue("topics")
    # selection = form.getvalue('selection')

    make_tag_generator_page()

    sys.exit(0)
