#!/usr/bin/python3
#
# Randal A. Koene, 20250513
#
# This script parses HTML that contains Log chunks and adds selection checkboxes with
# Log chunk IDs to easily select a subset of Log chunks for further processing, e.g. to
# extract related data and carry out processing.
#
# This script is a receiver script that can be used when calling fzloghtml
# with the -S (select and process) option.
#
# This script was made with the help of DeepSeek.

import cgi
import cgitb
import sys

# Enable error reporting for debugging (remove in production)
cgitb.enable()

def main():
    # Set the content type to plain text
    print("Content-Type: text/plain\n")
    
    try:
        # Parse the form data
        form = cgi.FieldStorage()
        
        # Initialize list to store select values
        select_values = []
        
        # Check if the form was submitted with POST
        if sys.stdin.isatty():
            print("Error: This script should be called with POST method.")
            return
        
        # Get all 'select' parameters (handles multiple values for the same name)
        if 'select' in form:
            # FieldStorage returns all values for a parameter as a list
            select_params = form.getlist('select')
            select_values.extend(select_params)
        
        # Print the collected values for demonstration
        print("Collected 'select' values:")
        for value in select_values:
            print(f"- {value}")
            
        # Here you would typically do further processing with select_values
        # For example: save to database, process the data, etc.
        
    except Exception as e:
        print(f"An error occurred: {str(e)}")

if __name__ == "__main__":
    main()
