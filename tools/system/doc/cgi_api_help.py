#!/usr/bin/python3
#
# Randal A. Koene, 20240930
#
# Generate an index to the built-in help of the Formalizer
# CGI scripts.

from os import listdir
from os.path import isfile, join

# Print this early to catch errors:
print("Content-type: text/html\n")

PAGETEMPLATE='''<html>
<head>
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
<title>fz: CGI API Help Index</title>
</head>
<body>
<script type="text/javascript" src="/fzuistate.js"></script>

<h1>fz: CGI API Help Index</h1>

<table><tbody>
%s
</tbody></table>

</body>
</html>
'''

LINETEMPLATE='''<tr>
<td>%s</td>
</tr>
'''

LINETEMPLATEWITHLINK='''<tr>
<td><a href="/cgi-bin/%s?help=on">%s</a></td>
</tr>
'''

# Specify the directory path
directory_path = "/usr/lib/cgi-bin"

# Get the list of all files and directories in the specified directory
files_and_dirs = listdir(directory_path)

# Filter for only files that are Python CGI scripts
files = [f for f in files_and_dirs if isfile(join(directory_path, f)) and '.py' in f]

# Filter for files with the 'help' option
files_with_help = []
for file in files:
    with open(directory_path+'/'+file, 'r') as f:
        scriptcontent = f.read()
        files_with_help.append(".getvalue('help')" in scriptcontent or '.getvalue("help")' in scriptcontent)

# Print the list of files
content = ''
for i in range(len(files)):
    if files_with_help[i]:
        content += LINETEMPLATEWITHLINK % (files[i], files[i])
    else:
        content += LINETEMPLATE % files[i]

print(PAGETEMPLATE % content)
