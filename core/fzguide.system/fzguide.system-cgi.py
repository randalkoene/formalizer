#!/usr/bin/python3
#
# Randal A. Koene, 20200923
#
# This CGI handler provides a near-verbatim equivalent access to fzguide.system via web form.

# Import modules for CGI handling 
try:
    import cgitb; cgitb.enable()
except:
    pass
import sys, cgi, os
sys.stderr = sys.stdout
from time import strftime
import traceback
from io import StringIO
from traceback import print_exc
from subprocess import Popen, PIPE

# cgitb.enable()
# cgitb.disable()
# cgitb.enable(display=0, logdir="/tmp/test_python_cgiformget.log")

pagecss = """
table.blueTable {
  border: 1px solid #1C6EA4;
  background-color: #EEEEEE;
  width: 100%;
  text-align: left;
  border-collapse: collapse;
}
table.blueTable td, table.blueTable th {
  border: 1px solid #AAAAAA;
  padding: 3px 2px;
}
table.blueTable tbody td {
  font-size: 13px;
}
table.blueTable tr:nth-child(even) {
  background: #D0E4F5;
}
table.blueTable tfoot td {
  font-size: 14px;
}
table.blueTable tfoot .links {
  text-align: right;
}
table.blueTable tfoot .links a{
  display: inline-block;
  background: #1C6EA4;
  color: #FFFFFF;
  padding: 2px 8px;
  border-radius: 5px;
}
"""

# Create instance of FieldStorage 
form = cgi.FieldStorage() 

# Get data from fields
submit = form.getvalue('submit')
chapter  = form.getvalue('chapter')
spec_chapter = form.getvalue('spec_chapter')
sectionnum  = form.getvalue('sectionnum')
spec_sectionnum = form.getvalue('spec_sectionnum')
subsectionnum  = form.getvalue('subsectionnum')
spec_subsectionnum = form.getvalue('spec_subsectionnum')
subsection  = form.getvalue('subsection')
spec_subsection = form.getvalue('spec_subsection')
decimalindex = form.getvalue('decimalindex')
spec_index = form.getvalue('spec_index')
content = form.getvalue('content')

print("Content-type:text/html\n\n")
print("<html>")
print("<head>")
print('<link rel="stylesheet" href="https://www.w3schools.com/w3css/4/w3.css">')
print("<title>Formalizer: HTML FORM interface to fzguide.system</title>")
print("<style>")
print(pagecss)
print("</style>")
print("</head>")
print("<body>")

thisscript = os.path.realpath(__file__)
print(f'(For dev reference, this script is at {thisscript}.)')

print("<h1>Formalizer: HTML FORM interface to fzguide.system</h1>\n<p></p>")
#print("<table>")

if (submit=="Read"):

    cmdoptions = ' -R '

    if spec_chapter:
        if (chapter=="AM"):
            cmdoptions += ' -A '
        else:
            cmdoptions += ' -P '

    if spec_sectionnum:
        cmdoptions += ' -H '+sectionnum
    if spec_subsectionnum:
        cmdoptions += ' -u '+subsectionnum
    if spec_subsection:
        cmdoptions += ' -U '+subsection
    if spec_index:
        cmdoptions += ' -x '+decimalindex

    if cmdoptions:
        thecmd = "./fzguide.system -q -d formalizer -s randalk -E STDOUT -F fullhtml "+cmdoptions
        print('Using this command: ',thecmd)
        print('<br>')
        try:
            p = Popen(thecmd,shell=True,stdin=PIPE,stdout=PIPE,close_fds=True, universal_newlines=True)
            (child_stdin,child_stdout) = (p.stdin, p.stdout)
            child_stdin.close()
            result = child_stdout.read()
            child_stdout.close()
            print('<table class="blueTable"><tbody>\n')
            print(result)
            print('\n</tbody></table>\n')
            #print(result.replace('\n', '<BR>'))

        except Exception as ex:
            print('<pre>\n')
            print(ex)
            f = StringIO()
            print_exc(file=f)
            a = f.getvalue().splitlines()
            for line in a:
                print(line)
            print('\n</pre>\n')


else:
    if (submit=="Store"):

        cmdoptions = ' -S '

        if (chapter=="AM"):
            cmdoptions += ' -A '
        else:
            cmdoptions += ' -P '

        cmdoptions += ' -H '+sectionnum
        cmdoptions += ' -u '+subsectionnum
        cmdoptions += ' -U '+subsection
        cmdoptions += ' -x '+decimalindex

        if cmdoptions:
            thecmd = "./fzguide.system -q -d formalizer -s randalk -E STDOUT -F html "+cmdoptions
            print('Using this command: ',thecmd)
            print('<br>\n')
            try:
                p = Popen(thecmd,shell=True,stdin=PIPE,stdout=PIPE,close_fds=True, universal_newlines=True)
                (child_stdin,child_stdout) = (p.stdin, p.stdout)
                #child_stdin.write(content) # Send the content to the server via STDIN
                (childstdout_data, childstderr_data) = p.communicate(content)  # docs recommend using this instead of write
                if childstdout_data:
                    print(childstdout_data)
                if childstderr_data:
                    print(childstderr_data)
                child_stdin.close()
                result = child_stdout.read()
                child_stdout.close()
                print(result)
                #print(result.replace('\n', '<BR>'))

            except Exception as ex:                
                print(ex)
                f = StringIO()
                print_exc(file=f)
                a = f.getvalue().splitlines()
                for line in a:
                    print(line)


    else:
        if (submit=='ListIDs'):
            thecmd = "./fzguide.system -q -d formalizer -s randalk -E STDOUT -F html -L"
            print('Using this command: ',thecmd)
            print('<br>\n')
            try:
                p = Popen(thecmd,shell=True,stdin=PIPE,stdout=PIPE,close_fds=True, universal_newlines=True)
                (child_stdin,child_stdout) = (p.stdin, p.stdout)
                child_stdin.close()
                result = child_stdout.read()
                child_stdout.close()
                print(result)

            except Exception as ex:                
                print(ex)
                f = StringIO()
                print_exc(file=f)
                a = f.getvalue().splitlines()
                for line in a:
                    print(line)

        else:
            print(f'Unrecognized form request: {submit}')


#if "name" not in form or "addr" not in form:
#    print("<H1>Error</H1>")
#    print("Please fill in the name and addr fields.")
#    return

#print("</table>")
print("</body>")
print("</html>")
