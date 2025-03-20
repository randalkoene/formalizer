#!/usr/bin/python3
#
# Randal A. Koene, 20250320
#
# This script receives the content of a checkbox line and uses that as the content
# for a new Node.

try:
    import cgitb; cgitb.enable()
except:
    pass
import sys, cgi, os
sys.stderr = sys.stdout
from io import StringIO
from traceback import print_exc
from urllib.parse import unquote

# Create instance of FieldStorage 
form = cgi.FieldStorage()

data = form.getvalue('data')
decoded_data = unquote(data)

print("Content-type:text/html\n\n")

TEST_CONTENT='''<html></body>
Content is:
%s
</body></html>
'''

def main():
    print(TEST_CONTENT % decoded_data)

if __name__ == "__main__":
    main()
