#! /usr/bin/python
"""

NAME

  httprouter.py - redirect HTTP requests to other servers

USAGE

  python httprouter.py local_port

DESCRIPTION

  sockproxy will accept TCP connections on local_port and forward them
  to remote_port on remote_host, printing traffic.

AUTHOR

  Noel Burton-Krahn <noel@burton-krahn.com>
  
HISTORY

  2006-04-10 - NBK - added docstrings and command-line handling

"""

import asyncore
import socket
import sys
import getopt
import re
import string
from hexdumper import hexdump
from listutil import fixlen
from email.Parser import HeaderParser
from traceback import format_tb 

class HttpRouter(asyncore.dispatcher):
    """
    accepts connections on local_port, then creates a pair of
    HttpRouterPeer's to forward and print traffic.
    """
    def __init__(self):
        asyncore.dispatcher.__init__(self)

    def open(self, local_port):
        local_port = port2int(local_port)
        self.local_port = local_port
        self.create_socket(socket.AF_INET, socket.SOCK_STREAM)
        self.bind(("0.0.0.0", local_port))
        self.listen(5)
        self.connection_count = 0
        self.local_addr = self.socket.getsockname()
        self.name = "%s" % (self.local_port)
        self.printd("listening on %s" % (self.local_addr,))
        
    def printd(self, msg, name=""):
        for s in msg.split("\n"):
            print(name + s)
        sys.stdout.flush()
        
    def handle_accept(self):
        (client_sock, client_addr) = self.accept()
        self.connection_count += 1

        self.printd("%s A%03d: %s" % (self.name, self.connection_count, client_addr))
        
        client = HttpRouterPeer(self, "%s C%03d: " % (self.name, self.connection_count))
        client.set_socket(client_sock)
        
    def handle_client(self, client):
        remote_host = "google.com"
        remote_port = 80

        # hack: take the first non-empty path element as the hostname
        req = client.request
        for s in string.split(req.path, '/'):
            if len(s) == 0: continue
            remote_host = s
            break

        client.connect_server((remote_host, remote_port,))

    def run(self):
        asyncore.loop()
        


class HttpReq:
    pass

http_header_re = re.compile(r'\s*(.*?)\r?\n\r?\n', re.M | re.S);

class HttpRouterPeer(asyncore.dispatcher):
    """
    One half of a bidirectional connection, either server or client.
    It reads from a socket, and writes to it's peer.
    """
    def __init__(self, parent, name):
        asyncore.dispatcher.__init__(self)
        self.name = name
        self.send_buf = ''
        self.send_eof = 0
        self.parent = parent
        self.header_buf = ''
        self.header_max = 16384
        self.peer = None

    def printd(self, data):
        self.parent.printd(data, self.name)
        
    def handle_connect(self):
        # debug
        self.printd("handle_connect")

    def handle_close(self):
        # debug
        self.printd("handle_close")
        pass
    
    def handle_read(self):
        # debug
        self.printd("handle_read")

        try:
            # read from my socket, printd it, and send to my peer
            data = self.recv(4096)
        except:
            self.printd("EXCEPTION in recv: value=%s traceback=\n%s\n"
                        % (sys.exc_value, ''.join(format_tb(sys.exc_traceback))))
            data = ""
        else: 
            # debug
            self.printd("handle_read: len(data)=%d" % (len(data),))
        
        if len(data) == 0:
            self.printd("CLOSED on read")
            self.close()

        # if I've already connected to the remote server, write immediately
        if self.peer:
            self.peer.write(data)
            return

        # try to parse a complete HTTP header
        self.header_buf += data
        hdr_match = http_header_re.match(self.header_buf)
        if not hdr_match:
            # not a complete header yet, keep reading
            if len(self.header_buf) < self.header_max:
                return

            # error! too much data without an HTTP header. Barf!
            self.close()
            return

        # debug
        self.printd("handle_read: self.header_buf=[%s]" % (self.header_buf,))

        try:
            req = HttpReq()
            self.request = req
            
            # parse the HTTP header and find the remote server
            headers = hdr_match.group(1);
            (req.reqline, headers) = fixlen(2, re.split(r'\r?\n', headers, 1))
            req.headers = HeaderParser().parsestr(headers)

            (req.method, req.path, req.version) = fixlen(3, string.split(req.reqline, None, 3))

            # debug
            self.printd('req=%s' % (req,))
            
        except:
            # debug
            self.printd("EXCEPTION in handle_read: value=%s traceback=\n%s\n"
                        % (sys.exc_value, ''.join(format_tb(sys.exc_traceback))))

        # my parent can decide where to redirect this client to (by calling connect_server)
        self.parent.handle_client(self)
            
    def connect_server(self, remote_addr):
        # debug
        self.printd("connecting to remote_addr=%s" % (remote_addr,))

        # start connecting to my other half
        parent = self.parent
        peer = HttpRouterPeer(parent, "%s S%03d: " % (parent.name, parent.connection_count))
        peer.create_socket(socket.AF_INET, socket.SOCK_STREAM)
        peer.connect(remote_addr)
        self.peer = peer
        peer.peer = self

        # send my buffered header to my peer
        self.peer.write(self.header_buf)

        
    def write(self, data):
        """
        called by my peer to send on my socket
        """
        if len(data) == 0:
            self.send_eof = 1
        else:
            self.send_buf += data
        
    def writable(self):
        return (self.send_eof or len(self.send_buf) > 0)
    
    def handle_write(self):
        """
        If I'm connected, write from my peer's recv buffer and
        drain it
        """
        if len(self.send_buf) > 0:
            sent = self.send(self.send_buf)
            if sent == 0:
                self.printd("CLOSED on send")
                self.send_buf = ''
                self.send_eof = 1
            elif sent > 0:
                self.send_buf = self.send_buf[sent:]
                
        if self.send_eof and len(self.send_buf) == 0:
            self.send_eof = 0
            self.socket.shutdown(socket.SHUT_WR)

def port2int(x):
    try:
        x = int(x)
    except:
        x = socket.getservbyname(x, 'tcp')
    return x
    
            
if __name__ == "__main__":
    opts, args = getopt.getopt(sys.argv[1:], "h")

    for o, a in opts:
        if o in ("-h", "-help"):
            print __doc__
            sys.exit(0)

    local_port = port2int(args[0])

    # debug
    print "local_port=%s" % local_port

    HttpRouter(local_port)
        
    asyncore.loop()
