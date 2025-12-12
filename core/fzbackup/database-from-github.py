#!/usr/bin/env python3
#
# Download the latest Formalizer database backed up on GitHub.

import requests
import os
import json

user_home = os.environ['HOME']

OWNER = "koenera"
REPO = "formalizer2"
DIRPATH = "archive/postgres"
with open(user_home+'.fzdatabase-token', 'r') as f:
    fzdatabase = json.load(f)
TOKEN = fzdatabase['fzdatabase']

API_URL = f"https://api.github.com/repos/{OWNER}/{REPO}/contents/{DIRPATH}"
HEADERS = {"Authorization": f"token {TOKEN}"}

response = requests.get(API_URL, headers=HEADERS).json()
files = [f for f in response if f["type"] == "file"]

if not files:
    raise RuntimeError("No files found.")

# Sort by filename descending
latest = sorted(files, key=lambda x: x["name"], reverse=True)[0]

url = latest["download_url"]
name = latest["name"]

print(f"Downloading {name} ...")
content = requests.get(url, headers=HEADERS).content

with open(user_home+'/.formalizer/archive/postgres/'+name, "wb") as f:
    f.write(content)

print("Saved as %s" % name)

exit(0)
