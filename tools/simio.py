#!/usr/bin/env python
# *****************************************************************************
#  Copyright (c) 2014, AllSeen Alliance. All rights reserved.
#
#     Permission to use, copy, modify, and/or distribute this software for any
#     purpose with or without fee is hereby granted, provided that the above
#     copyright notice and this permission notice appear in all copies.
#
#     THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
#     WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
#     MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
#     ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
#     WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
#     ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
#     OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
# *****************************************************************************

import Tkinter as Tk
import socket
import os

class Pin(object):
    """Pin abstraction - common functionality across input and output pins"""
    trigger_rising = 1
    trigger_falling = 2

    def __init__(self, val=0):
        #print "val: %s" % val
        self.val = val
        self.trigger = self.trigger_falling

    def setVal(self, val):
        #print "val: %s" % val
        self.val = val

    def getVal(self):
        return self.val

    def toggle(self):
        if self.getVal():
            self.setVal(0)
        else:
            self.setVal(1)

    def setTrigger(self, trigger):
        #print "trigger: %s" % trigger
        self.trigger = trigger

    def getTrigger(self):
        return self.trigger

class Led(Pin, Tk.Canvas):
    """Output pin displayed as a colored square"""
    def __init__(self, master, val=1, color="black"):
        Pin.__init__(self, val)
        Tk.Canvas.__init__(self, master, width=30, height=30)
        self.color = color
        self.draw()

    def draw(self):
        if self.getVal():
            color = self.color
        else:
            color = "black"
        self.create_rectangle(0, 0, 30, 30, fill=color)

    def setVal(self, val):
        Pin.setVal(self, val)
        self.draw()

class InputPin(Pin, Tk.Checkbutton):
    """Input pin displayed as a check box"""
    def __init__(self, master, val=0, text=""):
        Pin.__init__(self, val)
        self.tkint = Tk.IntVar()
        Tk.Checkbutton.__init__(self, master, text=text, variable=self.tkint)
        self.text = text
        self.tkint.trace('w', self.tkint_cb)
        self.setVal(self.tkint.get())

    def tkint_cb(self, *args):
        #print "%s: %s->%s" % (self.text, self.getVal(), self.tkint.get())
        self.setVal(self.tkint.get())

class InputButton(Pin, Tk.Button):
    """Edge-triggered input button, displayed as a button. Events are sent to the registered server"""
    def __init__(self, master, server, val=0, text=""):
        Pin.__init__(self, val)
        Tk.Button.__init__(self, master, text=text)
        self.server = server
        self.text = text
        self.bind('<Button-1>', self.down_cb)
        self.bind('<ButtonRelease-1>', self.up_cb)

    def down_cb(self, event):
        if self.getTrigger() == Pin.trigger_falling:
            #print "Falling"
            self.server.send_interrupt(self, Pin.trigger_falling)

    def up_cb(self, event):
        if self.getTrigger() == Pin.trigger_rising:
            #print "Rising"
            self.server.send_interrupt(self, Pin.trigger_rising)

class Simio(object):
    """
    Overall application object.
    
    Handles socket IO and GUI organization.
    """

    server_name = "/tmp/ajs_gui"  # Path to socket
    command_len = 3               # Bytes per incoming command
    columns = 4                   # Number of columns in pin display

    def __init__(self, master):
        self.master = master

        frame = Tk.Frame(master)
        frame.grid()

        self.server = None
        self.sock = None
        self.readbuf = ''

        self.server_setup()

        f = frame
        self.pins = [Led(f, color="red"),
                     Led(f, color="green"),
                     Led(f, color="yellow"),
                     Led(f, color="blue"),
                     InputPin(f, text='1'),
                     InputPin(f, text='2'),
                     InputPin(f, text='3'),
                     InputPin(f, text='4'),
                     InputButton(f, self, text='A'),
                     InputButton(f, self, text='B'),
                     InputButton(f, self, text='C'),
                     InputButton(f, self, text='D')]

        for i, pin in enumerate(self.pins):
            pin.grid(row=i/self.columns, column=i%self.columns, padx=20, pady=20)

        self.button = Tk.Button(frame, text="Quit", command=frame.quit)
        self.button.grid(columnspan=self.columns, padx=20, pady=20)

    def server_setup(self):
        """Set up incoming socket and listen for connections"""
        self.server = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.server.setblocking(0)
        
        try:
            os.remove(self.server_name)
        except OSError:
            pass

        self.server.bind(self.server_name)
        self.server.listen(1)
        
        self.master.createfilehandler(self.server, Tk.tkinter.READABLE, self.server_cb)

    def server_shutdown(self):
        """Disable the server socket"""
        if self.server is not None:
            self.master.deletefilehandler(self.server)
        self.server.close()
        self.server = None

    def server_accept(self):
        """Set up communication socket when a client tries to connect"""
        self.sock, address = self.server.accept()
        self.sock.setblocking(0)

        print "Accepted"
        
        self.master.createfilehandler(self.sock, Tk.tkinter.READABLE, self.sock_cb)

    def sock_recv(self):
        """Handle incoming data and hand off to command processor"""
        data = self.sock.recv(self.command_len)
        #print "sock read: %s" % repr(data)

        if len(data) == 0:
            self.sock_shutdown()
            print "socket closed"

        self.readbuf += data
        while len(self.readbuf) >= self.command_len:
            self.process_command(self.readbuf[:self.command_len])
            self.readbuf = self.readbuf[self.command_len:]

    def sock_shutdown(self):
        """Disable the communication socket"""
        if self.sock is not None:
            self.master.deletefilehandler(self.sock)
        self.sock.close()
        self.sock = None

    def send_interrupt(self, pin, trigger):
        """Issue an interrupt command to the client"""
        if trigger == Pin.trigger_falling:
            edge = 0
        else:
            edge = 1

        self.send_command('i', self.pins.index(pin)+1, edge)

    def send_command(self, command, pin, val):
        """Pack and send a command to the client"""
        if self.sock:
            data = command + chr(pin) + chr(val)
            #print "sock send: %s" % repr(data)
            self.sock.send(data)

    def process_command(self, command):
        """Parse and perform a single command"""
        op = command[0]
        pin = ord(command[1])
        val = ord(command[2])

        if pin >= len(self.pins):
            return

        if op == 'w':
            # Write
            self.pins[pin].setVal(val)
        elif op == 'r':
            # Read
            self.send_command('r', pin, self.pins[pin].getVal())
        elif op == 't':
            # Toggle
            self.pins[pin].toggle()
        elif op == 'i':
            # Interrupt trigger
            self.pins[pin].setTrigger(val)

    def server_cb(self, fno, mask):
        """Callback for when the server socket becomes readable (incoming connection)"""
        if mask & Tk.tkinter.READABLE:
            self.server_accept()

    def sock_cb(self, fno, mask):
        """Callback for when the data socket becomes readable (incoming data)"""
        if mask & Tk.tkinter.READABLE:
            self.sock_recv()

root = Tk.Tk()

simio = Simio(root)

root.mainloop()
root.destroy()
