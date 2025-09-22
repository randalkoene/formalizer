#!/usr/bin/python3
#
# Randal A. Koene, 20250922
#
# This script retrieve's the calendar events in the upcoming week.
#
# This script was made with the help of Google Genesis.


import os
import datetime
import os.path
import pathlib

from google.auth.transport.requests import Request
from google.oauth2.credentials import Credentials
from google_auth_oauthlib.flow import InstalledAppFlow
from googleapiclient.discovery import build
from googleapiclient.errors import HttpError

# If modifying these scopes, delete the file token.json.
SCOPES = ["https://www.googleapis.com/auth/calendar.readonly"]

home_directory = os.environ['HOME']
credentials_path = home_directory+'/.formalizer/config/daywiz/credentials.json'

def main():
    """Shows basic usage of the Google Calendar API.
    Lists the next week's events on the user's primary calendar.
    """
    creds = None
    # The file token.json stores the user's access and refresh tokens, and is
    # created automatically when the authorization flow completes for the first
    # time.
    if os.path.exists("token.json"):
        creds = Credentials.from_authorized_user_file("token.json", SCOPES)
    # If there are no (valid) credentials available, let the user log in.
    if not creds or not creds.valid:
        if creds and creds.expired and creds.refresh_token:
            creds.refresh(Request())
        else:
            # Check if credentials.json exists in the current directory.
            # This file must be downloaded from the Google Cloud Console.
            if not os.path.exists(credentials_path):
                print("Error: credentials.json not found.")
                print("Please download your credentials file from the Google Cloud Console and place it in the same directory as this script.")
                return

            flow = InstalledAppFlow.from_client_secrets_file(
                credentials_path, SCOPES
            )
            creds = flow.run_local_server(port=0)
        # Save the credentials for the next run
        with open("token.json", "w") as token:
            token.write(creds.to_json())

    try:
        service = build("calendar", "v3", credentials=creds)

        # Call the Calendar API
        now = datetime.datetime.utcnow().isoformat() + "Z"  # 'Z' indicates UTC time
        next_week = (datetime.datetime.utcnow() + datetime.timedelta(days=7)).isoformat() + "Z"
        
        print("Getting the upcoming week's events...")
        events_result = (
            service.events()
            .list(
                calendarId="primary",
                timeMin=now,
                timeMax=next_week,
                maxResults=10,
                singleEvents=True,
                orderBy="startTime",
            )
            .execute()
        )
        events = events_result.get("items", [])

        if not events:
            print("No upcoming events found.")
            return

        # Print the events
        print("Upcoming events for the next 7 days:")
        for event in events:
            start = event["start"].get("dateTime", event["start"].get("date"))
            end = event["end"].get("dateTime", event["end"].get("date"))
            
            # Format the output for readability
            summary = event.get("summary", "No Title")
            location = event.get("location", "No Location")
            
            print("-" * 30)
            print(f"Event: {summary}")
            print(f"Location: {location}")
            print(f"Starts: {start}")
            print(f"Ends:   {end}")
            print("-" * 30)

    except HttpError as error:
        print(f"An error occurred: {error}")

if __name__ == "__main__":
    main()
