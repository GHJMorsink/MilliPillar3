# -*- coding: utf-8 -*-
'''
Main program startup for the Stimulator
----------------------------------------

Contains the main configuration startup and starts all elements
'''


import sys
import time
#import ctypes
import traceback

from gui import GuiBuilder
from measuring import Measure, getDeviceList
from bootld import Bootload


class Application(object):
    ''' Contains the LRC application '''
    def __init__(self):
        ''' Application (root) object constructor '''
        self._guiBuilder = None
        self.Meas = None

    def getDevice(self):
        ''' try to find the device '''
        if self.Meas:
            self.Meas.getDevice()

    def doReset(self, fileName, comport):
        ''' request for a reset and boot loading '''
        if fileName == '':
            return
        if self.Meas:
            self.Meas.stop()
        time.sleep(0.2)
        del self.Meas
        self.Meas = None
        flasher = Bootload(fileName, comport)
        flasher.start_cl(self._guiBuilder.mw.UpdateScrStatus)
        del flasher
        time.sleep(2)   #wait for startup
        self._makeNewMeas(comport)


    def setPort(self, comport):
        ''' changing the comport
        '''
        if self.Meas:
            self.Meas.stop()
        time.sleep(0.2)
        del self.Meas
        self.Meas = None 
        self._makeNewMeas(comport)

       
    def _makeNewMeas(self, comport):
        ''' make a new instance for the comport measurement thread
        '''
        self.Meas = Measure(port=comport)   # start again and read settings
        self.Meas.setDsp(self._guiBuilder.mw.UpdateScrStatus)
        self._guiBuilder.mw.get_initial_data()


    def calibrate(self, value):
        ''' translate the user input to a calibration
        '''
        try:
            rawvalue = float(value)
            #print 'Read value:', rawvalue
            if rawvalue > 0:
                self.Meas.writeCalibration(0, rawvalue)
                self.Meas.writeCalibration(1, rawvalue)
        except:
            traceback.print_exc()

    def sendMsg(self, txt):
        ''' give a command to th device '''
        self.Meas.SendRequest(txt)


    def getversion(self):
        ''' Get the USB device version '''
        return self.Meas.version


    def start_gui(self):
        '''Start main GUI thread'''
        comlist,preferred = getDeviceList()
        self.Meas = Measure(port=comlist[preferred])
        #At this point other subthreads already started so we only start the GUI main thread
        self._guiBuilder = GuiBuilder( self )
        self.Meas.setDsp(self._guiBuilder.mw.UpdateScrStatus)
        self._guiBuilder.mw.setComlist(comlist, preferred) # this also gets initial data
        self._guiBuilder.start()

    def stop(self):
        ''' Stop all internal stuff '''
        self.Meas.stop()


def main():
    ''' Starts the application '''

    application = Application()

    try:
        application.start_gui()
    finally:
        if sys.stdout:
            sys.stdout.flush()
            sys.stderr.flush()
        application.stop()
        #ctypes.cdll.msvcrt.exit()


if __name__ == '__main__':
    main()
