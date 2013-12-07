import sys
import time
import socket
import threading

def listen_socket(ip, port):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.bind((ip, port))
    sock.listen(12) #backlog
    return sock

def open_socket(num, ip, port):

    sockets = []

    for i in range(0, num):
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:

            sock.connect((ip, port))
            sockets.append(sock)
        except:
            print "connect to " + ip + ":%d" % port + " failed."

    return sockets

def operate_socket(sockets, msg):

    for i in range(0, len(sockets)):
        sockets[i].send(msg + ":data from socket client:%d" % i)

def accept_proc(sock):

    while True:

        conn, adress = sock.accept()

        print "py client accept one socket!"

        msg = "py server got you! welcome!"
        conn.send(msg)

        try:
            buf = conn.recv(1024)
            if buf == msg:
                print "py client recv:" + buf
                conn.close()
        except:
            print "exception on recv of accepted socket" 

class socket_thread(threading.Thread):

    def __init__(self, proc, arg):

        self.proc = proc
        self.arg  = arg
        threading.Thread.__init__(self)

    def run(self):
        self.proc(self.arg)

if __name__ == '__main__':

    ip = sys.argv[1]
    port = int(sys.argv[2])
    listen = int(sys.argv[3])

    listen_sock = listen_socket("localhost", listen)
    sockets = open_socket(23, ip, port)

    accept_thread = socket_thread(accept_proc, listen_sock)
    accept_thread.start()

    for i in range(1, 3):
        operate_socket(sockets, "py test client:127.0.0.1:%d:%d" % (listen, i))

    time.sleep(1)

    for i in range(0, len(sockets)):
        buf = sockets[i].recv(1024)
        print "recv:" + buf + "\n"

    accept_thread.join()

    for i in range(0, len(sockets)):
        sockets[i].close()


