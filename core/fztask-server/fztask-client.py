import json

import sseclient

# This is just an example of how a Python program can become a listener. Of course,
# any program that can create a TCP client connection on the requisite port can also
# do so.
#
# An obvious thing to do is to make fztask-server the curator and arbitrator of
# task chunk management, and to allow fztask.py to be a listener.
# Meanwhile, javascript code in a browser can also be a listener.
#
# For fztask.py to use fztask-server, subscribe as shown below to be alerted
# to Task Chunk changes elicited by others. Also, import fztaskAPI.py in order to
# make Task Chunk change signaling calls from fztask.py.

if __name__ == '__main__':

    messages = sseclient.SSEClient('http://localhost:5000/listen')

    for msg in messages:
        print(msg)  # call print(dir(msg)) to see the available attributes
