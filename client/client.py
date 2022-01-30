#! /usr/bin/env python3
#-*- coding:utf-8 -*-

import socket
import time
import base64
import select 
import sys
import fcntl
import os
import re
import struct
class BBS_client:
    def __init__(self, server_addr):
        self.username = b''
        self.client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.chat_recv = None
        self.profile = {'name' : b'', 'port' : -1, 'version': -1}
        self.server_addr = server_addr
        
        # bind
        self.client.connect(server_address)

        # set non-blocking
        self.client.setblocking(False)

        # unblocking stdin
        stdin_fd = sys.stdin.fileno()
        fl = fcntl.fcntl(stdin_fd, fcntl.F_GETFL)
        fcntl.fcntl(stdin_fd, fcntl.F_SETFL, fl | os.O_NONBLOCK)

        # create epoll object
        self.epoll = select.epoll()
        self.epoll.register(self.client.fileno(), select.EPOLLIN)
        self.epoll.register(stdin_fd, select.EPOLLIN)

        # chat buffer
        self.data = b''
    def clean(self):
        if self.chat_recv:
            self.chat_recv.close()

        self.client.close()

    def chat(self, msg):
        name = self.profile['name']
        v = self.profile['version']
        data = b''
        if v == 1:
            data = b'\x01\x01' + struct.pack(b'>H', len(name)) + name + struct.pack(b'>H', len(msg)) + msg
        else:
            data = b'\x01\x02' + base64.b64encode(name) + b'\n' + base64.b64encode(msg) + b'\n'
        self.chat_recv.sendto(data, self.server_addr)
        
    def _extract_message(self):
        if len(self.data) <= 0:
            return b''
        out = b''
        try:
            flag = self.data[0]
            if flag == 1:
                version = self.data[1]
                if version == 1:
                    name_len = struct.unpack('>H', self.data[2:4])[0]
                    name = self.data[4:4+name_len]
                    message_len = struct.unpack('>H', self.data[4+name_len:6+name_len])[0]
                    message = self.data[6+name_len:6+name_len+message_len]

                    l = name_len + message_len + 6       
                    
                    if len(self.data) > l:
                        self.data = self.data[l:]
                    else:
                        self.data = b""

                    out = name + b':' + message + b'\n% ' 

                elif version == 2:
                    tmp = self.data[2:].strip().split(b'\n')
                    b64_name = tmp[0]
                    b64_message = tmp[1]
                    name = base64.b64decode(b64_name)
                    message = base64.b64decode(b64_message)

                    l = len(b64_name) + len(b64_message) + 4       
                    
                    if len(self.data) > l:
                        self.data = self.data[l:]
                    else:
                        self.data = b""

                    out = name + b':' + message + b'\n% ' 
                else:
                    return b""
            else:
                return b""
        except:
            return b""

        return out


           
    def recv_chat(self):
        while True:
            tmp = b""
            try:
                tmp = self.chat_recv.recv(2048)
                self.data += tmp
            except:
                break
        while self.data:
            out = self._extract_message()
            sys.stdout.write(out.decode())


    def send(self, instr):
        self.client.sendall(instr)

    def recv(self):
        data = self.client.recv(1000)
        if len(data) <= 0:
            self.clean()
            sys.exit(0)
        while True:
            try:
                d = self.client.recv(1000)
                data += d 
                if len(d) <= 0:
                    raise Exception('')
            except:
                m = re.search(b'^Welcome, (.*).',data.strip())
                if m:
                    self.profile['name'] = m.group(1) 
                m = re.search(b'^Welcome to public chat room.\nPort:(.*)\nVersion:(.*)\n', data)
                if m:
                    self.profile['port'] = int(m.group(1))
                    self.profile['version'] = int(m.group(2))
                    if self.chat_recv == None:
                        self.chat_recv = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
                        self.chat_recv.setblocking(False)
                        self.epoll.register(self.chat_recv.fileno(), select.EPOLLIN)
                        self.chat_recv.bind(("127.0.0.1", self.profile['port']))
                    else:
                        self.epoll.unregister(self.chat_recv.fileno())
                        self.chat_recv.close()
                        self.chat_recv = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
                        self.chat_recv.setblocking(False)
                        self.epoll.register(self.chat_recv.fileno(), select.EPOLLIN)
                        self.chat_recv.bind(("127.0.0.1", self.profile['port']))


                m = re.search(b'^Bye, (.*).\n% $', data)
                if m:
                    if self.chat_recv != None:
                        self.epoll.unregister(self.chat_recv.fileno())
                        self.chat_recv.close()
                        self.chat_recv = None

                    self.profile['name'] = b''
                    self.profile['port'] = -1
                    self.profile['version'] = -1
                sys.stdout.write(data.decode())
                break
            
        return True

if len(sys.argv) != 2:
    print("Usage: %s <port>" %(sys.argv[0]))
    sys.exit(-1)
server_address = ("127.0.0.1", int(sys.argv[1]))

bbs_client = BBS_client(server_address)


while True:
    events = bbs_client.epoll.poll(10)
    if not events:
        #print("No events")
        continue
    
    #print("%d events come in"%(len(events)))

    for fd, event in events:
        # User input
        if fd == sys.stdin.fileno():
            instrs = []
            try:
                instrs = sys.stdin.readlines()
            except:
                pass
            if instrs == []:
                continue

            instrs = [i.encode() for i in instrs]
            for instr in instrs:
                try:
                    cmd = [i for i in instr.strip().split(b' ') if i]
                    if cmd[0] in [b'register', b'login', b'logout', b'exit', b'enter-chat-room']:
                        bbs_client.send(b' '.join(cmd) + b'\n')
                    elif cmd[0] == b'chat':
                        message = instr.strip().replace(b'chat ',b'')
                        bbs_client.chat(message)
                    else:
                        pass

                except:
                    pass

        # Connection close
        elif event & select.EPOLLHUP:
            bbs_client.clean()
            sys.exit(0)

        # Readable event
        elif event & select.EPOLLIN:
            if fd == bbs_client.client.fileno():
                if not bbs_client.recv():
                   sys.exit(0) 
            else:
                bbs_client.recv_chat()

        elif event & select.EPOLLOUT:
            pass

