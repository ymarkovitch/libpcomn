#!/usr/bin/env python
# FILE         :  echoserver-stream.py
# COPYRIGHT    :  Yakov Markovitch, 2008-2014. All rights reserved.
#                 See LICENSE for information on usage/redistribution.
#
# DESCRIPTION  :  A simple echo server, using stream sockets
# CREATION DATE:  21 Jan 2008
"""A simple echo server, using stream sockets
"""

import socket, sys, time

def run(host = '', port = 50000, accept_delay = 0, receive_delay = 0, send_delay = 0, backlog = 5):
    print "Echo server started on host %r, port %d, backlog %d. Accept delay: %s, receive delay: %s, send delay: %s" % \
        (host, port, backlog, accept_delay, receive_delay, send_delay)
    size = 1024
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind((host,port))
    s.listen(backlog)
    while True:
        time.sleep(accept_delay)
        print "Accepting..."
        client, address = s.accept()
        print "Accepted. Address: %s" % (address, )
        time.sleep(receive_delay)
        print "Receiving..."
        data = client.recv(size)
        print "Received %d bytes." % (data and len(data) or 0)
        if data:
            time.sleep(send_delay)
            print "Sending...",
            client.send(data)
            print "Sent"
        print "Closing..."
        client.close()
        print "Closed"
    return True

if __name__ == "__main__":
    len(sys.argv) <= 1 and run() or eval(sys.argv[1])
