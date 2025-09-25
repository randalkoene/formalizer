#!/usr/bin/python3
#
# Counts the exact number of unread emails in the Inbox.
#

from __future__ import print_function
import os

from google.auth.transport.requests import Request
from google.oauth2.credentials import Credentials
from google_auth_oauthlib.flow import InstalledAppFlow
from googleapiclient.discovery import build
from googleapiclient.errors import HttpError

SCOPES = [
    "https://www.googleapis.com/auth/calendar.readonly",
    "https://www.googleapis.com/auth/gmail.readonly"
]

home_directory = os.environ['HOME']
token_path = home_directory + '/.formalizer/config/daywiz/token.json'
credentials_path = home_directory + '/.formalizer/config/daywiz/credentials.json'

def main():
    """
    Counts and prints the exact number of unread emails in the Inbox.
    """
    creds = None
    if os.path.exists(token_path):
        creds = Credentials.from_authorized_user_file(token_path, SCOPES)

    if not creds or not creds.valid:
        if creds and creds.expired and creds.refresh_token:
            creds.refresh(Request())
        else:
            print('Looking for credentials in: ' + credentials_path)
            if not os.path.exists(credentials_path):
                print("Error: credentials.json not found.")
                return
            flow = InstalledAppFlow.from_client_secrets_file(credentials_path, SCOPES)
            creds = flow.run_local_server(port=0)
        with open(token_path, "w") as token:
            token.write(creds.to_json())

    try:
        service = build("gmail", "v1", credentials=creds)

        count = 0
        next_page_token = None

        while True:
            response = (
                service.users()
                .messages()
                .list(
                    userId="me",
                    #labelIds=["INBOX", "UNREAD"],
                    maxResults=500,  # up to 500 per page
                    q="in:inbox is:unread",
                    pageToken=next_page_token
                )
                .execute()
            )

            messages = response.get("messages", [])
            count += len(messages)

            next_page_token = response.get("nextPageToken")
            if not next_page_token:
                break

        # Note that this result isn't necessarily the same as the number of
        # threads with unread emails in them.
        print(f"Unread emails: {count}")

    except HttpError as error:
        print(f"An error occurred: {error}")
    except Exception as e:
        print(f"An unexpected error occurred: {e}")

if __name__ == "__main__":
    main()
