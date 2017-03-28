#!/usr/bin/env python2
# -*- coding: utf-8 -*-
##################################################
# GNU Radio Python Flow Graph
# Title: Top Block
# Generated: Tue Mar 28 14:38:08 2017
##################################################

if __name__ == '__main__':
    import ctypes
    import sys
    if sys.platform.startswith('linux'):
        try:
            x11 = ctypes.cdll.LoadLibrary('libX11.so')
            x11.XInitThreads()
        except:
            print "Warning: failed to XInitThreads()"

from PyQt4 import Qt
from gnuradio import analog
from gnuradio import blocks
from gnuradio import digital
from gnuradio import eng_notation
from gnuradio import filter
from gnuradio import gr
from gnuradio import qtgui
from gnuradio import uhd
from gnuradio.eng_option import eng_option
from gnuradio.filter import firdes
from gnuradio.qtgui import Range, RangeWidget
from optparse import OptionParser
import chan_change
import dsmx
import math
import sip
import sys
import threading
import time


class top_block(gr.top_block, Qt.QWidget):

    def __init__(self):
        gr.top_block.__init__(self, "Top Block")
        Qt.QWidget.__init__(self)
        self.setWindowTitle("Top Block")
        try:
            self.setWindowIcon(Qt.QIcon.fromTheme('gnuradio-grc'))
        except:
            pass
        self.top_scroll_layout = Qt.QVBoxLayout()
        self.setLayout(self.top_scroll_layout)
        self.top_scroll = Qt.QScrollArea()
        self.top_scroll.setFrameStyle(Qt.QFrame.NoFrame)
        self.top_scroll_layout.addWidget(self.top_scroll)
        self.top_scroll.setWidgetResizable(True)
        self.top_widget = Qt.QWidget()
        self.top_scroll.setWidget(self.top_widget)
        self.top_layout = Qt.QVBoxLayout(self.top_widget)
        self.top_grid_layout = Qt.QGridLayout()
        self.top_layout.addLayout(self.top_grid_layout)

        self.settings = Qt.QSettings("GNU Radio", "top_block")
        self.restoreGeometry(self.settings.value("geometry").toByteArray())

        ##################################################
        # Variables
        ##################################################
        self.samp_rate = samp_rate = 2e6
        self.intermediate_samp_rate = intermediate_samp_rate = samp_rate
        self.top_block_fournisseur = top_block_fournisseur = 0
        self.sps = sps = int(intermediate_samp_rate/1e6)
        self.nfilts = nfilts = 32
        self.f_deviation = f_deviation = 1e6
        self.channel = channel = 13

        ##################################################
        # Blocks
        ##################################################
        self._channel_range = Range(0, 100, 1, 13, 1)
        self._channel_win = RangeWidget(self._channel_range, self.set_channel, "channel", "counter_slider", int)
        self.top_layout.addWidget(self._channel_win)
        self.chan_change = chan_change.blk(param="self")
        self.uhd_usrp_source_0 = uhd.usrp_source(
        	",".join(("", "")),
        	uhd.stream_args(
        		cpu_format="fc32",
        		channels=range(1),
        	),
        )
        self.uhd_usrp_source_0.set_samp_rate(samp_rate)
        self.uhd_usrp_source_0.set_center_freq(((channel+1) * 1e6) + 2.4e9, 0)
        self.uhd_usrp_source_0.set_normalized_gain(1, 0)
        self.uhd_usrp_source_0.set_antenna('TX/RX', 0)
        
        def _top_block_fournisseur_probe():
            while True:
                val = self.chan_change.set_top_block(self)
                try:
                    self.set_top_block_fournisseur(val)
                except AttributeError:
                    pass
                time.sleep(1.0 / (1))
        _top_block_fournisseur_thread = threading.Thread(target=_top_block_fournisseur_probe)
        _top_block_fournisseur_thread.daemon = True
        _top_block_fournisseur_thread.start()
            
        self.qtgui_time_sink_x_1_1 = qtgui.time_sink_f(
        	12000, #size
        	1e6, #samp_rate
        	"", #name
        	2 #number of inputs
        )
        self.qtgui_time_sink_x_1_1.set_update_time(0.10)
        self.qtgui_time_sink_x_1_1.set_y_axis(-1, 1)
        
        self.qtgui_time_sink_x_1_1.set_y_label('Amplitude', "")
        
        self.qtgui_time_sink_x_1_1.enable_tags(-1, True)
        self.qtgui_time_sink_x_1_1.set_trigger_mode(qtgui.TRIG_MODE_TAG, qtgui.TRIG_SLOPE_POS, 1.5, .0001, 0, "PreambleFound")
        self.qtgui_time_sink_x_1_1.enable_autoscale(False)
        self.qtgui_time_sink_x_1_1.enable_grid(False)
        self.qtgui_time_sink_x_1_1.enable_axis_labels(True)
        self.qtgui_time_sink_x_1_1.enable_control_panel(False)
        
        if not True:
          self.qtgui_time_sink_x_1_1.disable_legend()
        
        labels = ['', '', '', '', '',
                  '', '', '', '', '']
        widths = [1, 1, 1, 1, 1,
                  1, 1, 1, 1, 1]
        colors = ["blue", "red", "green", "black", "cyan",
                  "magenta", "yellow", "dark red", "dark green", "blue"]
        styles = [1, 1, 1, 1, 1,
                  1, 1, 1, 1, 1]
        markers = [-1, -1, -1, -1, -1,
                   -1, -1, -1, -1, -1]
        alphas = [1.0, 1.0, 1.0, 1.0, 1.0,
                  1.0, 1.0, 1.0, 1.0, 1.0]
        
        for i in xrange(2):
            if len(labels[i]) == 0:
                self.qtgui_time_sink_x_1_1.set_line_label(i, "Data {0}".format(i))
            else:
                self.qtgui_time_sink_x_1_1.set_line_label(i, labels[i])
            self.qtgui_time_sink_x_1_1.set_line_width(i, widths[i])
            self.qtgui_time_sink_x_1_1.set_line_color(i, colors[i])
            self.qtgui_time_sink_x_1_1.set_line_style(i, styles[i])
            self.qtgui_time_sink_x_1_1.set_line_marker(i, markers[i])
            self.qtgui_time_sink_x_1_1.set_line_alpha(i, alphas[i])
        
        self._qtgui_time_sink_x_1_1_win = sip.wrapinstance(self.qtgui_time_sink_x_1_1.pyqwidget(), Qt.QWidget)
        self.top_layout.addWidget(self._qtgui_time_sink_x_1_1_win)
        self.qtgui_time_sink_x_1_0 = qtgui.time_sink_f(
        	int(12240.0*sps/8.0), #size
        	samp_rate, #samp_rate
        	"", #name
        	1 #number of inputs
        )
        self.qtgui_time_sink_x_1_0.set_update_time(0.10)
        self.qtgui_time_sink_x_1_0.set_y_axis(-1, 1)
        
        self.qtgui_time_sink_x_1_0.set_y_label('Amplitude', "")
        
        self.qtgui_time_sink_x_1_0.enable_tags(-1, True)
        self.qtgui_time_sink_x_1_0.set_trigger_mode(qtgui.TRIG_MODE_FREE, qtgui.TRIG_SLOPE_POS, 1.5, 0, 0, "Found")
        self.qtgui_time_sink_x_1_0.enable_autoscale(False)
        self.qtgui_time_sink_x_1_0.enable_grid(False)
        self.qtgui_time_sink_x_1_0.enable_axis_labels(True)
        self.qtgui_time_sink_x_1_0.enable_control_panel(False)
        
        if not True:
          self.qtgui_time_sink_x_1_0.disable_legend()
        
        labels = ['', '', '', '', '',
                  '', '', '', '', '']
        widths = [1, 1, 1, 1, 1,
                  1, 1, 1, 1, 1]
        colors = ["blue", "red", "green", "black", "cyan",
                  "magenta", "yellow", "dark red", "dark green", "blue"]
        styles = [1, 1, 1, 1, 1,
                  1, 1, 1, 1, 1]
        markers = [-1, -1, -1, -1, -1,
                   -1, -1, -1, -1, -1]
        alphas = [1.0, 1.0, 1.0, 1.0, 1.0,
                  1.0, 1.0, 1.0, 1.0, 1.0]
        
        for i in xrange(1):
            if len(labels[i]) == 0:
                self.qtgui_time_sink_x_1_0.set_line_label(i, "Data {0}".format(i))
            else:
                self.qtgui_time_sink_x_1_0.set_line_label(i, labels[i])
            self.qtgui_time_sink_x_1_0.set_line_width(i, widths[i])
            self.qtgui_time_sink_x_1_0.set_line_color(i, colors[i])
            self.qtgui_time_sink_x_1_0.set_line_style(i, styles[i])
            self.qtgui_time_sink_x_1_0.set_line_marker(i, markers[i])
            self.qtgui_time_sink_x_1_0.set_line_alpha(i, alphas[i])
        
        self._qtgui_time_sink_x_1_0_win = sip.wrapinstance(self.qtgui_time_sink_x_1_0.pyqwidget(), Qt.QWidget)
        self.top_layout.addWidget(self._qtgui_time_sink_x_1_0_win)
        self.dsmx_preambleDetection_0 = dsmx.preambleDetection(channel)
        self.dsmx_Despreader_0 = dsmx.Despreader()
        self.digital_correlate_access_code_tag_bb_0 = digital.correlate_access_code_tag_bb('1100110011001100110011001100110011001100110011001100110011001100', 8, 'PreambleFound')
        self.digital_clock_recovery_mm_xx_1 = digital.clock_recovery_mm_ff(sps, 1, 0.5, 0.175, 0.005)
        self.digital_binary_slicer_fb_1 = digital.binary_slicer_fb()
        self.blocks_char_to_float_0_1 = blocks.char_to_float(1, 1)
        self.analog_quadrature_demod_cf_1 = analog.quadrature_demod_cf(1)

        ##################################################
        # Connections
        ##################################################
        self.msg_connect((self.dsmx_Despreader_0, 'pdus'), (self.chan_change, 'ChannelChange'))    
        self.msg_connect((self.dsmx_preambleDetection_0, 'pdus'), (self.dsmx_Despreader_0, 'Msg'))    
        self.connect((self.analog_quadrature_demod_cf_1, 0), (self.digital_clock_recovery_mm_xx_1, 0))    
        self.connect((self.analog_quadrature_demod_cf_1, 0), (self.qtgui_time_sink_x_1_0, 0))    
        self.connect((self.blocks_char_to_float_0_1, 0), (self.qtgui_time_sink_x_1_1, 0))    
        self.connect((self.digital_binary_slicer_fb_1, 0), (self.digital_correlate_access_code_tag_bb_0, 0))    
        self.connect((self.digital_clock_recovery_mm_xx_1, 0), (self.digital_binary_slicer_fb_1, 0))    
        self.connect((self.digital_clock_recovery_mm_xx_1, 0), (self.qtgui_time_sink_x_1_1, 1))    
        self.connect((self.digital_correlate_access_code_tag_bb_0, 0), (self.blocks_char_to_float_0_1, 0))    
        self.connect((self.digital_correlate_access_code_tag_bb_0, 0), (self.dsmx_preambleDetection_0, 0))    
        self.connect((self.uhd_usrp_source_0, 0), (self.analog_quadrature_demod_cf_1, 0))    

    def closeEvent(self, event):
        self.settings = Qt.QSettings("GNU Radio", "top_block")
        self.settings.setValue("geometry", self.saveGeometry())
        event.accept()

    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate
        self.uhd_usrp_source_0.set_samp_rate(self.samp_rate)
        self.qtgui_time_sink_x_1_0.set_samp_rate(self.samp_rate)
        self.set_intermediate_samp_rate(self.samp_rate)

    def get_intermediate_samp_rate(self):
        return self.intermediate_samp_rate

    def set_intermediate_samp_rate(self, intermediate_samp_rate):
        self.intermediate_samp_rate = intermediate_samp_rate
        self.set_sps(int(self.intermediate_samp_rate/1e6))

    def get_top_block_fournisseur(self):
        return self.top_block_fournisseur

    def set_top_block_fournisseur(self, top_block_fournisseur):
        self.top_block_fournisseur = top_block_fournisseur

    def get_sps(self):
        return self.sps

    def set_sps(self, sps):
        self.sps = sps
        self.digital_clock_recovery_mm_xx_1.set_omega(self.sps)

    def get_nfilts(self):
        return self.nfilts

    def set_nfilts(self, nfilts):
        self.nfilts = nfilts

    def get_f_deviation(self):
        return self.f_deviation

    def set_f_deviation(self, f_deviation):
        self.f_deviation = f_deviation

    def get_channel(self):
        return self.channel

    def set_channel(self, channel):
        self.channel = channel
        self.uhd_usrp_source_0.set_center_freq(((self.channel+1) * 1e6) + 2.4e9, 0)
        self.dsmx_preambleDetection_0.set_new_channel(self.channel)


def main(top_block_cls=top_block, options=None):

    from distutils.version import StrictVersion
    if StrictVersion(Qt.qVersion()) >= StrictVersion("4.5.0"):
        style = gr.prefs().get_string('qtgui', 'style', 'raster')
        Qt.QApplication.setGraphicsSystem(style)
    qapp = Qt.QApplication(sys.argv)

    tb = top_block_cls()
    tb.start()
    tb.show()

    def quitting():
        tb.stop()
        tb.wait()
    qapp.connect(qapp, Qt.SIGNAL("aboutToQuit()"), quitting)
    qapp.exec_()


if __name__ == '__main__':
    main()
