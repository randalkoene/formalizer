#!/usr/bin/python3
#
# This tests how to add a gate to a CGI script so that it does not
# rerun when the page is reloaded.
#
# Randal A. Koene, 20241030

try:
    import cgitb; cgitb.enable()
except:
    pass
import sys, cgi, os
sys.stderr = sys.stdout
import traceback
from io import StringIO
from traceback import print_exc

from proclock import called_as_cgi, make_runstamp, ReloadGate

form = cgi.FieldStorage()

print("Content-type: text/html\n")

RELOAD_GATE_BLOCKED='''<html><body>
<h1>Reload Gate - Blocked</h1>

<p>
Stored gate stamp: %s
</p>

<p>
CGI gate stamp: %s
</p>

</body></html>
'''

RELOAD_GATE_PASSED='''<html><body>
<h1>Reload Gate - Passed</h1>

<p>
%s
</p>

<p>
Possible errors: %s
</p>

</body></html>
'''

def reload_blocked(gate:ReloadGate):
    print(RELOAD_GATE_BLOCKED % (gate.storedstamp, gate.runstamp))

GATED_SCRIPT_CALLER='''<html><body>
<h1>Call Gated Script</h1>

<p>
<a href="/cgi-bin/test_noreload_cgi.py?runstamp=%s">Click here</a>
</p>

</body></html>
'''

if __name__ == '__main__':
    caller = form.getvalue('caller')
    if caller:
        # Call the script that is gated.
        runstamp = make_runstamp()
        print(GATED_SCRIPT_CALLER % runstamp)
        sys.exit(0)

    else:
        # Test the gate.
        reloadgate = ReloadGate(form, '/tmp/test_noreload_cgi.stamp', reload_blocked)
        if called_as_cgi():
            print(RELOAD_GATE_PASSED % ('CGI arguments: '+str(list(form.keys())), reloadgate.stamperror))
        else:
            print(RELOAD_GATE_PASSED % ('User: '+str(os.environ["USER"], reloadgate.stamperror)))
        sys.exit(0)
