#!/usr/bin/python3
#
# Counts the exact number of unread emails in the Inbox and optionally
# lists subjects from the last 24h.

from __future__ import print_function
import os
import argparse

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

# def get_recent_subjects(service):
#     """
#     Fetches and prints subject lines for emails received in the last 24 hours.
#     """
#     print("\n--- Subjects from the last 24 hours ---")
#     try:
#         # Query for messages from the last 1 day
#         results = service.users().messages().list(userId='me', q='newer_than:1d').execute()
#         messages = results.get('messages', [])

#         if not messages:
#             print("No messages found from the last 24 hours.")
#             return

#         for msg in messages:
#             msg_data = service.users().messages().get(userId='me', id=msg['id']).execute()
#             headers = msg_data['payload']['headers']
#             subject = next((header['value'] for header in headers if header['name'] == 'Subject'), 'No Subject')
#             print(f"- {subject}")
            
#     except Exception as e:
#         print(f"Error fetching subjects: {e}")

def get_recent_details(service):
    """
    Fetches and prints Senders and Subject lines for emails from the last 24 hours.
    """
    print("\n--- Emails from the last 24 hours ---")
    try:
        # Query for messages from the last 1 day
        results = service.users().messages().list(userId='me', q='newer_than:1d').execute()
        messages = results.get('messages', [])

        if not messages:
            print("No messages found from the last 24 hours.")
            return

        for msg in messages:
            msg_data = service.users().messages().get(userId='me', id=msg['id'], format='metadata', metadataHeaders=['From', 'Subject']).execute()
            headers = msg_data.get('payload', {}).get('headers', [])
            
            # Extract specific headers
            subject = next((h['value'] for h in headers if h['name'] == 'Subject'), '(No Subject)')
            sender = next((h['value'] for h in headers if h['name'] == 'From'), '(Unknown Sender)')
            
            print(f"From: {sender}")
            print(f"Subj: {subject}")
            print("-" * 30)
            
    except Exception as e:
        print(f"Error fetching details: {e}")

def main():
    # Set up command line arguments
    parser = argparse.ArgumentParser(description="Count unread emails and optionally list recent subjects.")
    parser.add_argument('-s', '--subjects', action='store_true', help="Show subject lines for emails from the last 24 hours")
    args = parser.parse_args()

    creds = None
    if os.path.exists(token_path):
        creds = Credentials.from_authorized_user_file(token_path, SCOPES)

    if not creds or not creds.valid:
        if creds and creds.expired and creds.refresh_token:
            creds.refresh(Request())
        else:
            print('Looking for credentials in: ' + credentials_path)
            if not os.path.exists(credentials_path):
                print(f"Error: credentials.json not found in {credentials_path}")
                return
            flow = InstalledAppFlow.from_client_secrets_file(credentials_path, SCOPES)
            creds = flow.run_local_server(port=0)
        with open(token_path, "w") as token:
            token.write(creds.to_json())

    try:
        service = build("gmail", "v1", credentials=creds)

        # Original Functionality: Count unread
        count = 0
        next_page_token = None
        while True:
            response = service.users().messages().list(
                userId="me",
                #labelIds=["INBOX", "UNREAD"],
                maxResults=500,  # up to 500 per page
                q="in:inbox is:unread",
                pageToken=next_page_token
            ).execute()

            messages = response.get("messages", [])
            count += len(messages)
            next_page_token = response.get("nextPageToken")
            if not next_page_token:
                break

        # Note that this result isn't necessarily the same as the number of
        # threads with unread emails in them.
        print(f"Unread emails: {count}")

        # New Functionality: Pull subjects if flag is provided
        if args.subjects:
            get_recent_details(service)

    except HttpError as error:
        print(f"An error occurred: {error}")
    except Exception as e:
        print(f"An unexpected error occurred: {e}")

if __name__ == "__main__":
    main()
