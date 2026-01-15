#!/usr/bin/python3

import os

print("Content-type: text/html\n\n")

print("<meta http-equiv=\"cache-control\" content=\"no-cache\" />")

#print(os.getlogin())

#print("Yes, it does still add text. So, the missing USER info is weird...")

print("<font size=+1>Environment</font><br>")
for param in os.environ.keys():
   print("<b>%20s</b>: %s<br>" % (param, os.environ[param]))

f = open("/tmp/fromCGI.txt", "w")
f.write("Now the file has more content!")
f.close()
