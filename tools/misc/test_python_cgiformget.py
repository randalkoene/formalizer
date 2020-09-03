#!/usr/bin/python3
#
# Randal A. Koene, 20200902

# Import modules for CGI handling 
import cgi, cgitb

# cgitb.enable()
# cgitb.disable()
# cgitb.enable(display=0, logdir="/tmp/test_python_cgiformget.log")

# Create instance of FieldStorage 
form = cgi.FieldStorage() 

# Get data from fields
node_id = form.getvalue('node_id')
logchunk_id  = form.getvalue('logchunk_id')

print("Content-type:text/html\n\n")
print("<html>")
print("<head>")
print("<title>Test Prototype: Request a Node and Log Chunk</title>")
print("</head>")
print("<body>")
print("<h1>Test Prototype: Request a Node and Log Chunk</h1>\n<p></p>")
print("<h2>Node %s and Log Chunk %s</h2>" % (node_id, logchunk_id))

#if "name" not in form or "addr" not in form:
#    print("<H1>Error</H1>")
#    print("Please fill in the name and addr fields.")
#    return

print("</body>")
print("</html>")
