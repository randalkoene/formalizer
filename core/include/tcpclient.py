# Copyright 2020 Randal A. Koene
# License TBD

"""
This header file declares functions used by client scripts to communicate
with Formalizer servers.

Versioning is based on https://semver.org/. See coreversion.hpp for more.
"""

import socket
from error import *


# default local Formalizer server IP address and port
serverIPport = ''


def get_server_address(fzuserbase: str):
    # Obtain IP address and port of locally running fzserverpq
    global serverIPport
    fzserveraddresspath = fzuserbase + '/server_address'
    try:
        with open(fzserveraddresspath) as f:
            serverIPport = f.read()
    except FileNotFoundError:
        print('Local server IP address and port file not found. Using default.\n')
        serverIPport = "127.0.0.1:8090"
    return serverIPport


def client_socket_request(request_str: str):
    if not serverIPport:
        exit_error(1, 'Unspecified server address and port. Call get_server_address() first.')
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    ipportvec = serverIPport.split(':')
    s.connect((ipportvec[0], int(ipportvec[1])))
    s.send(request_str.encode())
    data = ''
    data = s.recv(1024).decode()
    return data


# Takes a request string without the preceding 'FZ ' code.
def serial_API_request(request_str: str):
    res = client_socket_request(f'FZ {request_str}')
    d = res.split()
    d_len = len(d)
    if ((d_len < 2) or (d[0] != 'FZ')):
        exit_error(1, 'Server did not respond to FZ request as expected.')
    if (d[1] != '200'):
        exit_error(1, f'Server returned FZ error response {d[1]}.')
    if (d_len < 3):
        return ''
    if (d_len < 4):
        return d[2]
    return d[2:]
        
