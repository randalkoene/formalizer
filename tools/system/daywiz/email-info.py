#!/usr/bin/python3
#
# Randal A. Koene, 20250922
#
# This script counts the number of unread emails.
#
# This script was made with the help of Google Genesis.

# -*- coding: utf-8 -*-
"""
Shows how to count the number of unread emails in a Gmail account using the
Gmail API.

This script requires a one-time setup:
1. Go to the Google Cloud Console: https://console.cloud.google.com/
2. Create a new project or select an existing one.
3. Enable the Gmail API for your project.
   (In 'APIs & Services' -> 'Library', search for 'Gmail API' and enable it).
4. Create credentials for a "Desktop app".
   (In 'APIs & Services' -> 'Credentials', click 'Create Credentials' -> 'OAuth client ID').
5. Download the `credentials.json` file and place it in the same directory as this script.
   Make sure you have added yourself as a test user in the OAuth Consent Screen configuration.
6. Install the required libraries:
   pip install --upgrade google-api-python-client google-auth-httplib2 google-auth-oauthlib
"""
from __future__ import print_function

import os
import os.path
import sys

from google.auth.transport.requests import Request
from google.oauth2.credentials import Credentials
from google_auth_oauthlib.flow import InstalledAppFlow
from googleapiclient.discovery import build
from googleapiclient.errors import HttpError

# If modifying these scopes, delete the file token.pickle.
# The 'gmail.readonly' scope is sufficient for this task.
SCOPES = ["https://www.googleapis.com/auth/gmail.readonly"]

home_directory = os.environ['HOME']
credentials_path = home_directory+'/.formalizer/config/daywiz/credentials.json'

def main():
    """
    Counts and prints the number of unread emails.
    """
    creds = None
    # The file token.pickle stores the user's access and refresh tokens, and is
    # created automatically when the authorization flow completes for the first
    # time.
    if os.path.exists("token.pickle"):
        with open("token.pickle", "rb") as token:
            creds = Credentials.from_authorized_user_file("token.pickle", SCOPES)

    # If there are no (valid) credentials available, let the user log in.
    if not creds or not creds.valid:
        if creds and creds.expired and creds.refresh_token:
            creds.refresh(Request())
        else:
            print('Looking for credentials in: '+credentials_path)
            if not os.path.exists(credentials_path):
                print("Error: 'credentials.json' file not found.")
                print("Please follow the setup instructions in the comments of this script.")
                sys.exit(1)
            flow = InstalledAppFlow.from_client_secrets_file(credentials_path, SCOPES)
            creds = flow.run_local_server(port=0)
        # Save the credentials for the next run
        with open("token.pickle", "wb") as token:
            token.write(creds.to_json().encode())

    try:
        # Call the Gmail API
        service = build("gmail", "v1", credentials=creds)

        # The 'UNREAD' label is built-in to Gmail and tracks unread messages.
        # We use the 'list' method with the 'labelIds' parameter to filter.
        # The 'resultSizeEstimate' property gives us the total count without fetching all messages.
        results = (
            service.users()
            .messages()
            .list(userId="me", labelIds=["UNREAD"])
            .execute()
        )
        
        # Get the estimated total number of results
        count = results.get("resultSizeEstimate", 0)

        print(f"You have {count} unread emails.")
        
    except HttpError as error:
        print(f"An error occurred: {error}")
    except Exception as e:
        print(f"An unexpected error occurred: {e}")


if __name__ == "__main__":
    main()
