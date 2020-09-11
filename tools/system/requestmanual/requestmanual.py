#!/usr/bin/python3
#
# requestmanual.py
#
# Randal A. Koene, 20200904
#
# Request user action. See the README.md file for details.

import argparse

parser = argparse.ArgumentParser(description='Request manual user action.')
parser.add_argument('actionrequest', metavar='action', nargs='+', help='a description of the action to take')

args = parser.parse_args()

print('Please carry out this action manually, then confirm when it has been completed.\n')

print('Requested action:\n')


for actionline in args.actionrequest:
    print("\t",actionline)

completed = input('\nPlease press ENTER when the requested action has been completed...\n')
