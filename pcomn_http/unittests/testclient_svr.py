#!/usr/bin/env python
# FILE         :  testclient_svr.py
# DESCRIPTION  :  A simple HTTP/1.1 server, maps current directory to an index
# COPYRIGHT    :  Yakov Markovitch, 2010-2014. All rights reserved.
#                 See LICENSE for information on usage/redistribution.
#
# CREATION DATE:  13 May 2010
"""A simple HTTP/1.1 server for testing a client, maps current directory to an index
"""

import BaseHTTPServer, SimpleHTTPServer, sys, os, os.path

def run(protocol="HTTP/1.1"):
    """Test the HTTP request handler class.

    This runs an HTTP server on port 8000 (or the first command line
    argument).

    """

    if sys.argv[1:]:
        port = int(sys.argv[1])
    else:
        port = 8000
    server_address = ('localhost', port)

    handler_class = SimpleHTTPServer.SimpleHTTPRequestHandler
    handler_class.protocol_version = protocol
    httpd = BaseHTTPServer.HTTPServer(server_address, handler_class)

    sa = httpd.socket.getsockname()
    print "Serving HTTP on", sa[0], "port", sa[1], "directory", os.getcwd(), "..."
    httpd.serve_forever()

if __name__ == "__main__":
    os.chdir(os.path.dirname(sys.argv[0]))
    run()
