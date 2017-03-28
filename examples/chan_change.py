"""
Embedded Python Blocks:

Each time this file is saved, GRC will instantiate the first class it finds
to get ports and parameters of your block. The arguments to __init__  will
be the parameters. All of them are required to have default values!
"""

import numpy as np
from gnuradio import gr
import pmt
import ttk as tk
import os, struct, array
from fcntl import ioctl
import sys, tty, termios
import threading


class blk(gr.sync_block):  # other base classes are basic_block, decim_block, interp_block
    """Controller block:
Recieves decoded data, prints it, follows the channel sequence and sweeps channels 3 to 11 when no signal is detected."""

    def __init__(self, param=1):  # only default arguments here
        """arguments to this function show up as parameters in GRC"""
        gr.sync_block.__init__(
            self,
            name='Channel Changer',   # will show up in GRC
            in_sig=[],
            out_sig = []
        )
        # if an attribute with the same name as a parameter is found,
        # a callback is registered (properties work, too).
        self.message_port_register_in(pmt.intern("ChannelChange"))
        self.set_msg_handler(pmt.intern("ChannelChange"), self.chanChangeCallback)
        self.top_block = param
        self.channel_data = np.zeros((2,7))
        self.startup = True
        self.next_channel = 3


        """Called when a decoded packet is recieved"""
    def chanChangeCallback(self, msg):
        string = ''

        meta = pmt.car(msg) #Extract the metadata
        self.next_channel = pmt.to_python(pmt.dict_ref(meta,pmt.intern("Next channel"),pmt.PMT_NIL))
        self.top_block.set_channel(self.next_channel)   #This call should be done as soon as possible to avoid issues with channel switch delays

        radio_id1 = pmt.to_python(pmt.dict_ref(meta,pmt.intern("Radio id1"),pmt.PMT_NIL))
        radio_id1 = pmt.to_python(pmt.dict_ref(meta,pmt.intern("Radio id1"),pmt.PMT_NIL))
        data = pmt.cdr(msg)         #Data should contain a 23 byte array with the channel sequence to follow.
        if not pmt.is_u8vector(data):
            print 'Invalid message type'
            return
        for item in pmt.to_python(data):
            string = string + str(item) + ' '               #update GUI
            self.window.detectedLabel.config(text = string)
        string = ''
        for i in range(0,7):
            tmp = pmt.to_python(pmt.dict_ref(meta,pmt.intern("0 "+str(i)),pmt.PMT_NIL))
            if tmp:
                self.channel_data[0][i] = tmp
            tmp = pmt.to_python(pmt.dict_ref(meta,pmt.intern("1 "+str(i)),pmt.PMT_NIL))     #Extract data coming from a potential 2nd packed (if more than 7 channels are being transmitted) unused
            if tmp:
                self.channel_data[1][i] = tmp
            string = string + "CH" + str(i) + ": " + '{:03.2f}'.format((self.channel_data[0][i]/1024.0) -1) + " | "
        self.window.channelInfoLabel.config(text = string)      #update GUI
        self.has_detected_signal = True

    """Called once a second"""
    def set_top_block(self, block):
        if self.startup:                    #Inititalize. If done in the __init__ function, GRC will try to call it before runtime and will crash (or do something bad) GRC does not like treads and don't know top_block
            self.has_detected_signal = True
            self.top_block = block
            self.startup = False
            def launchWindow():                 #Setup GUI
                self.window = ui(self)
                self.window.master.title('DSMx Decoder')
                self.window.mainloop()
            self._window_thread = threading.Thread(target=launchWindow)
            self._window_thread.daemon = True
            self._window_thread.start()

        if not self.has_detected_signal:
            print 'No signal found in 1s, changing channel'
            self.next_channel = self.sweep_channel(self.next_channel)
            self.window.detectedLabel.config(text = "Sweeping.")
            self.window.channelInfoLabel.config(text = "Current channel: "+str(self.next_channel))
            self.top_block.set_channel(self.next_channel)
        self.has_detected_signal = False

    def sweep_channel(self, current):
        current = current + 1
        if current > 11:
            current = 3
        return current

    def work(self, input_items, output_items): #Useless
        return len(input_items[0])



class ui(tk.Frame):
    def __init__(self, calling, master=None):
        tk.Frame.__init__(self, master)
        self.config(height = 100, width = 1500)
        self.grid()
        self.createWidgets()
        self.calling = calling

    def createWidgets(self):
        self.freqLabel = tk.Label()
        self.freqLabel.grid()
        self.detectedLabel = tk.Label(text=" ")
        self.detectedLabel.grid()
        self.channelInfoLabel = tk.Label(text=" No signal found..  ")
        self.channelInfoLabel.grid()
        self.freqLabel.bind_all('<KeyPress-Return>', self._enterHandler)        #Unused but useful in case of user interaction
        self.freqLabel.bind_all('<KeyPress-Escape>', self._escapeHandler)

    def _enterHandler(self,event):
        self.calling.input_key = 'enter'
    def _escapeHandler(self,event):
        self.calling.input_key = 'escape'
