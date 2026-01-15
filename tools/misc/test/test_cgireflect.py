#!/usr/bin/python3
#
# This test script receives CGI input and reflects back what was
# run and with what arguments, both command line arguments and
# CGI arguments if available.

import sys
import os
import cgi
import html

def handle_command_line():
    """
    Handles execution as a command line script using sys.argv.
    """
    # Simple argument parsing for illustration
    # Expected usage: python script_name.py [--name NAME] [--greeting GREETING]
    result = "Command line arguments:\n"

    for arg in sys.argv:
        result += arg + ','

    print(result)

def handle_cgi():
    """
    Handles execution as a CGI script using the cgi module.
    """
    # Parse form data
    form = cgi.FieldStorage()

    if len(form)==0:
        result = "No form data received."
    else:
        result = "Received Form Data:\n"
        # Loop through each field name (key) in the form object
        for field_name in form.keys():
            # Retrieve all values associated with the field name (handles multiple checkboxes/selects)
            field_values = form.getlist(field_name)

            # Sanitize the output to prevent XSS vulnerabilities
            safe_field_name = html.escape(field_name)
            safe_field_values = [html.escape(value) for value in field_values]

            result += f"{safe_field_name}:{', '.join(safe_field_values)}\n"
    
    # Print necessary CGI headers and HTML content
    print(f"{result}")

if __name__ == "__main__":
    # Check for the 'REQUEST_METHOD' environment variable to detect CGI
    handle_command_line()
    if 'REQUEST_METHOD' in os.environ:
        handle_cgi()
