#! /usr/bin/python
"""

NAME

  sockproxy - proxy local connections to a remote host and dump traffic

USAGE

  sockproxy local_port remote_host remote_port

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
from hexdumper import *

class SockProxyServer(asyncore.dispatcher):
    """
    accepts connections on local_port, then creates a pair of
    SockProxyConnectionHalf's to forward and print traffic.
    """
    def __init__(self, local_port, remote_host, remote_port):
        asyncore.dispatcher.__init__(self)
        self.remote_host = remote_host
        self.remote_port = remote_port
        self.create_socket(socket.AF_INET, socket.SOCK_STREAM)
        self.bind(("0.0.0.0", local_port))
        self.listen(5)
        self.connection_count = 0
        self.local_addr = self.socket.getsockname()
        self.name = "%s:%s" % (self.remote_host, self.remote_port)
        self.printd("listening on %s" % (self.local_addr,))
        
    def printd(self, msg, name=""):
        if isbinary(msg):
            msg = "<binary: %s>" % (len(msg),) # hexdump(msg)
            pass
        for s in msg.split("\n"):
            print(name + s)
        sys.stdout.flush()
        
    def handle_accept(self):
        (client_sock, client_addr) = self.accept()
        self.connection_count += 1

        self.printd("%s A%03d: %s" % (self.name, self.connection_count, client_addr))
        
        client = SockProxyConnectionHalf(self, "%s C%03d: " % (self.name, self.connection_count))
        client.set_socket(client_sock)
        
        server = SockProxyConnectionHalf(self, "%s S%03d: " % (self.name, self.connection_count))
        server.create_socket(socket.AF_INET, socket.SOCK_STREAM)
        server.connect((self.remote_host, self.remote_port))

        client.other_half = server
        server.other_half = client

class SockProxyConnectionHalf(asyncore.dispatcher):
    """
    One half of a bidirectional connection.  It reads from a socket,
    prints traffic, and writes to it's other half's write buffer.
    """
    def __init__(self, parent, name):
        asyncore.dispatcher.__init__(self)
        self.name = name
        self.send_buf = ''
        self.send_eof = 0
        self.parent = parent

    def printd(self, data):
        self.parent.printd(data, self.name)
        
    def handle_connect(self):
        self.printd("connected")

    def handle_close(self):
        pass
    
    def handle_read(self):
        # read from my socket, printd it, and send to my other half
        data = self.recv(8192)
        self.other_half.write(data)
        if len(data) == 0:
            self.close()
            data = "CLOSED on read"
        self.printd(data)
        
    def write(self, data):
        """
        called by my other half to send on my socket
        """
        if len(data) == 0:
            self.send_eof = 1
        else:
            self.send_buf += data
        
    def writable(self):
        return (self.send_eof or len(self.send_buf) > 0)
    
    def handle_write(self):
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
    try:
        opts, args = getopt.getopt(sys.argv[1:], "h")

        for o, a in opts:
            if o in ("-h", "-help"):
                print __doc__
                sys.exit(0)

        while len(args) > 0:
            (local_port, remote_host, remote_port) = args[0:3]
            args = args[3:]

            print "args=%s" % ((local_port, remote_host, remote_port),)
            
            (local_port, remote_port) = map(port2int, (local_port, remote_port))
            SockProxyServer(local_port, remote_host, remote_port)
        
    except Exception, e:
        print "Error: %s" % e
        print
        print "USAGE: sockproxy local_port remote_host remote_port"
        print
        sys.exit(1)

    asyncore.loop()
