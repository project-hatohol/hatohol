#!/usr/bin/env python

import os
import sys
import socket
import platform

if __name__=="__main__":
    test_socket = socket.socket(socket.AF_UNIX)
    test_socket.bind("/tmp/nagios.sock")
    test_socket.listen(1)

    pid = os.fork()
    if pid > 0:
        sys.exit(0)

    while True:
        connect, address = test_socket.accept()
        request = connect.recv(512)
        if not request:
            connect.close()
            continue

        msg = '[{"test": "message"}]'
        connect.send(msg)
        connect.close()
        continue
