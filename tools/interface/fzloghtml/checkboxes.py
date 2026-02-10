#!/usr/bin/python3
#
# Randal A. Koene, 20250320
#
# This script parses HTML that contains checkboxes and adds buttons to easily
# export their content to another script for further processing, e.g. to
# convert it into a Node.
#
# This script was made with the help of DeepSeek.

try:
    import cgitb; cgitb.enable()
except:
    pass
import sys, cgi, os
sys.stderr = sys.stdout
from io import StringIO
from traceback import print_exc

import subprocess
import html
from html.parser import HTMLParser
import base64

# Create instance of FieldStorage 
form = cgi.FieldStorage()

weeksinterval = form.getvalue('weeksinterval')
if not weeksinterval:
    weeksinterval = 24

print("Content-type:text/html\n\n")

PAGE_FRAME_TOP='''<html>
<head>
<meta charset="utf-8" />
<link rel="stylesheet" href="/fz.css">
<link rel="icon" href="/favicon-logentry-32x32.png">
<link rel="stylesheet" href="/fzuistate.css">
<link rel="stylesheet" href="/clock.css">
<link rel="stylesheet" href="/score.css">
<link rel="stylesheet" href="/copiedalert.css">
<link rel="stylesheet" href="/htmltemplatestocopy.css">
<link rel="stylesheet" href="/tooltip.css">
<meta http-equiv="cache-control" content="no-cache" />
<title>fz: Log interval - Checkboxes</title>
<style>
.logstate {
position: fixed;
top: 120px;
right: 0px;
font-size: 34px;
font-family: calibri;
}
#protocol_tab {
position: fixed;
top: 185px;
right: 0px;
width: 300px;
height: 500px;
display: block;
text-align: right;
}
.prot_tip {
width: 300px;
top: 100%;
right: 50%;
}
</style>
</head>
<body>
<script type="text/javascript" src="/clock.js"></script>
<script type="text/javascript" src="/score.js"></script>
<script type="module" src="/fzuistate.js"></script>
<button id="timerBarText" class="button button2 logstate">_____</button>

<div id="protocol_tab">
<button class="button button1" onclick="window.open('/cgi-bin/schedule-cgi.py?c=true&num_days=7&s=20', '_blank');">Calendar Schedule</button><br>
<span class="alt_tooltip">Distractions off <input type="checkbox">
<span class="alt_tooltiptext prot_tip"><div>Turn off video distractions.</div></span>
</span><br>
<span class="alt_tooltip">Brush away emotion <input type="checkbox">
<span class="alt_tooltiptext prot_tip"><div>Literally, brush away emotions, especially anxiety.</div></span>
</span><br>
<span class="alt_tooltip">Just get started <input type="checkbox">
<span class="alt_tooltiptext prot_tip"><div>Just get started on a Node, you don't need to complete it right away, you just need to figure out what to do and take notes about that to get started.</div></span>
</span><br>
<button class="button button2" onclick="window.open('/cgi-bin/orderscore-cgi.py', '_blank');">OrderScore</button><br>
<button class="button button1" onclick="window.open('/cgi-bin/fzloghtml-cgi.py?review=today', '_blank');">Today Review</button>
</div>

<script type="text/javascript" src="/stateoflog.js"></script>
<script type="text/javascript" src="/hoveropentab.js"></script>
<script>
const state_of_log = new logState(logstate_id="timerBarText");
const protocol_tab = new HoverOpenTab('timerBarText', 'protocol_tab');
</script>
<h1>Formalizer: HTML FORM interface to fzloghtml - Checkboxes</h1>
<p></p>
<table id="LogInterval"><tbody>
'''

PAGE_FRAME_BOTTOM='''</tbody></table>
<a name="END">&nbsp;</a>
<hr>

<p>[<a href="/index.html">fz: Top</a>] <span id="logautoupdate">_</span></p>

<script type="text/javascript" src="/delayedpopup.js"></script>
<script>
set_hover_delayed_function('.hoverdelayfunc', enlargeImage, 1000);
set_hover_delayed_function('.docpopupfunc', openPopup, 1000);
pastePopupLink('docpopupfunc', 'entrytext');
</script>
<script type="text/javascript" src="/logautoupdate.js"></script>
<script>
const global_autologupdate = new autoLogUpdate('logautoupdate', true, 'entrytext');
</script>
<!-- <script type="text/javascript" src="/logcheckbox.js"></script> -->
</body>
</html>
'''
class CheckboxParser(HTMLParser):
    def __init__(self):
        super().__init__()
        self.modified_html = []
        self.current_checkbox = False
        self.captured_content = []
        self.stop_tags = {'li', 'p'}  # Tags that stop content capture

    def handle_starttag(self, tag, attrs):
        if tag == 'input':
            attrs_dict = dict(attrs)
            if attrs_dict.get('type') == 'checkbox':
                # Check if the checkbox is NOT checked
                if 'checked' not in attrs_dict:
                    self.current_checkbox = True
                    # Add a button before the checkbox
                    self.modified_html.append(f'<button onclick="sendData(\'\')">MkNode</button>')
                # Append the checkbox tag to the modified HTML (but don't capture it)
                attrs_str = ' '.join(f'{k}="{v}"' for k, v in attrs)
                self.modified_html.append(f'<{tag} {attrs_str}>')
                return  # Skip further processing for the checkbox tag
        
        # If we're capturing content, append the start tag and its attributes
        if self.current_checkbox:
            attrs_str = ' '.join(f'{k}="{v}"' for k, v in attrs)
            self.captured_content.append(f'<{tag} {attrs_str}>')
        
        # If we're capturing content and encounter a stop tag, stop capturing
        if self.current_checkbox and tag in self.stop_tags:
            self.current_checkbox = False
            # Update the button's onclick event with the captured content
            content = ''.join(self.captured_content).strip()
            if content:
                # Properly URL-safe-base64-encode the content
                encoded_content = base64.urlsafe_b64encode(content.encode()).decode()
                self.modified_html = [
                    line.replace('sendData(\'\')', f'sendData(\'{encoded_content}\')')
                    if 'sendData(\'\')' in line else line
                    for line in self.modified_html
                ]
            self.captured_content = []
        
        # Append the current tag to the modified HTML
        attrs_str = ' '.join(f'{k}="{v}"' for k, v in attrs)
        self.modified_html.append(f'<{tag} {attrs_str}>')

    def handle_data(self, data):
        if self.current_checkbox:
            # Capture the visible content after the checkbox
            self.captured_content.append(data)
        
        # Append the data to the modified HTML
        self.modified_html.append(data)

    def handle_endtag(self, tag):
        if self.current_checkbox:
            # If we encounter a stop tag, stop capturing (but don't include the tag)
            if tag in self.stop_tags:
                self.current_checkbox = False
                # Update the button's onclick event with the captured content
                content = ''.join(self.captured_content).strip()
                if content:
                    # Properly URL-safe-base64-encode the content
                    encoded_content = base64.urlsafe_b64encode(content.encode()).decode()
                    self.modified_html = [
                        line.replace('sendData(\'\')', f'sendData(\'{encoded_content}\')')
                        if 'sendData(\'\')' in line else line
                        for line in self.modified_html
                    ]
                self.captured_content = []
            else:
                # Capture the end tag (if not a stop tag)
                self.captured_content.append(f'</{tag}>')
        
        # Append the end tag to the modified HTML
        self.modified_html.append(f'</{tag}>')

    def get_modified_html(self):
        return ''.join(self.modified_html)

def add_buttons_to_unchecked_checkboxes(html_content):
    parser = CheckboxParser()
    parser.feed(html_content)
    return parser.get_modified_html()

# Add JavaScript function to handle the button click and send data to the CGI script
JS_SCRIPT='''
<script>
function sendData(data) {
    // Open a new browser window with the CGI script's URL and pass the data as a query parameter
    var cgiScriptUrl = "/cgi-bin/fzgraphhtml-cgi.py?edit=new&data=" + encodeURIComponent(data);
    window.open(cgiScriptUrl, '_blank');
}
</script>
'''

def main():
    # Call the external program fzloghtml with the specified arguments
    regex_pattern = r'[<]input type="checkbox"[ ]*[>]'
    command = [
        './fzloghtml', '-q', '-d', 'formalizer', '-s', 'randalk', '-o', 'STDOUT', '-E', 'STDOUT', '-N',
        '-w', str(weeksinterval), '-x', regex_pattern
    ]
    
    try:
        result = subprocess.run(command, capture_output=True, text=True, check=True)
        html_content = result.stdout
    except subprocess.CalledProcessError as e:
        print(f"Error calling fzloghtml: {e}", file=sys.stderr)
        print(f"Command output (stderr): {e.stderr}", file=sys.stderr)
        sys.exit(1)
    
    # Add buttons to unchecked checkboxes
    modified_html = add_buttons_to_unchecked_checkboxes(html_content)
        
    # Combine the modified HTML with the JavaScript
    final_html = modified_html + JS_SCRIPT
    
    # Output the final HTML content
    print(PAGE_FRAME_TOP)
    print(final_html)
    print(PAGE_FRAME_BOTTOM)

if __name__ == "__main__":
    main()
