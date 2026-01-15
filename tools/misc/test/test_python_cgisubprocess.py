#!/usr/bin/python3
import subprocess

a = subprocess.check_output(["date"]) # this is your command

#print os_date
print("Content-type:text/html\r\n\r\n")
print('<html>')
print('<head>')
print('<title>Hello Word!</title>')
print('</head>')
print('<body>')
print('<h2>Hello Word! Today is: %s</h2>' % a)
print('</body>')
print('</html>')
