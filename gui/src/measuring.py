# -*- coding: utf-8 -*-
'''
USB measuring and setting Driver 
-------------------------

The driver will check connections;
it will do initializing at start
it will do regular measurement-update
Responses are read through an independent thread.

'''
import traceback
import time
import threading
from threading import Thread
import serial
import serial.tools.list_ports
import os

RETRYCOUNT = 3
MAXCHECKDISCONNECTED = 1000     # number of polling times to try a reconnect (--> 5 sec)
ESC = 0x1B
LF = 0x0A
CR = 0x0D


CMD_CALSET = 3
CMD_CALREAD = 4


def getDeviceList():
    ''' make a list of all serial ports in the system and give the port
        with USB as preferred
    '''
    ports = serial.tools.list_ports.comports()
    # 'com_list' contains list of all com ports

    com_list = []
    index = 0
    preferred = 0
    pref_found = False

    for p in ports:
        com_list.append(p.device)
        if p.description.find("USB") >= 0 and not pref_found:
            preferred = index
            pref_found = True
        index += 1
    return (com_list, preferred)
            


class Measure(Thread):  #pylint: disable=E1101
    ''' classdocs measurement
    The communication interface to the embedded stimulator
    '''
    TIMEDELAY = 0.005  # every 5ms a update polling time between measurements

    def __init__(self, port, baudrate = 38400):
        ''' Constructor '''
        Thread.__init__( self, target=self.read_serial)
        self.setName("Stimulator-serial")

        self.deviceName = port
        self.dspfunction = None                         # connected dsplay function
        self.setDaemon(True)
        self._lock = threading.Lock()
        self._pollingtime = Measure.TIMEDELAY
        self._stop = False
        self.receivemsg = ''
        self.bus = None
        ''' Try to open the serial port
        '''
        if self.bus:
            self.bus.close()
        retry = 0
        maxretry = RETRYCOUNT
    
        while retry < maxretry and self.bus == None:
            try:
                # print("Serial Try opening: " + self.deviceName)
                bytesize = serial.EIGHTBITS
                parity = serial.PARITY_NONE
                self.bus = serial.Serial(self.deviceName, baudrate=baudrate, bytesize=bytesize,
                                    parity=parity, rtscts=False, xonxoff=False, timeout=0.01 )
                #Note: on the (Beckhoff) virtual serial ports a timeout of zero is not allowed!!!!
            except:
                #All exceptions are trapped: not only serial, but also 'os' (wrong port name, etc)
                retry += 1
                self.bus = None
                if retry >= maxretry:
                    print("Serial didn't start, check configuration!")
                    print(traceback.format_exc())
                    return
            else:
                time.sleep(0.1)
        if self.bus.isOpen():
            self.bus.close()
        self.bus.open()
        self.connected = True
        self.start()    # start the thread

    def read_serial(self):
        ''' 
            This is the main thread, running on a time-element of TIMEDELAY second. 
        '''
        count = 0
        
        while not self._stop:
            if self.connected and self.getdata():
                self.checkdata()
            if not self.connected:
                count += 1
                if count > MAXCHECKDISCONNECTED:
                    count = 0
                    # self.tryreconnect()        # Try to reconnect to the port TODO!

            time.sleep(self._pollingtime)  # wait  for speedy stopping
        try:
            self.bus.close()        # if stopped, close the serial port
        except:
            pass
        

    def setDsp(self, statusf):
        ''' set the connection to the display '''
        self.dspfunction = statusf


    def stop(self):
        ''' Stopping this thread '''
        self._stop = True


    def SendRequest(self, reqtype):
        ''' Send a request message
        '''
        data = reqtype + '\n\r'
        self._sendmsg(data)
 

    def readCalibrations(self, offset): #TODO!
        ''' Read the calibration value according to the current mode and offset '''
        if not self.setCommandbuffer(CMD_CALREAD):
            return 0xFFFF
        rlen = 0
        count = 0
        while rlen == 0 and count < 2:
            count += 1
            self.buffer[2] = offset
            self.report[0].set_raw_data(self.buffer)
            self.report[0].send()
            rbuffer = self.report[0].get()
            print('calbuffer:', rbuffer)
            rlen = len(rbuffer)
            if rlen > 0:
                return rbuffer[3] * 256 + rbuffer[4]
        return 0xFFFF


    def writeCalibrations(self, offset, rawvalue): #TODO!
        ''' Write a calibration value according to current setting
            Input: the user value as given; calibration value still to be calculated
        '''
        if not self.setCommandbuffer(CMD_CALSET):
            return
        newcalvalue = int(rawvalue * 1000.0)
        self.buffer[2] = offset
        self.rangecalibrations[offset] = newcalvalue
        self.buffer[3] = int(newcalvalue / 256) & 0xFF
        self.buffer[4] = newcalvalue & 0xFF
        print('New calibration:', newcalvalue)
        self.report = self.device.find_feature_reports()
        self.report[0].set_raw_data(self.buffer)
        self.report[0].send()


    def _sendmsg(self, cv_msg):
        '''Send the complete message through the serial port
        '''
        
        if self.bus != None:
            self.bus.write(cv_msg.encode())
              

    def getdata(self):
        ''' Try to receive from line
        '''
        data = ''
        try:
            data = self.bus.read(1)
        except:     # serial.SerialTimeoutException (and also disconnection)
            self.connected = False
            return False
        if len(data) == 0:
            return False
        try:
            self.receivemsg = self.receivemsg + data.decode("utf-8")
        except:
            return False    # ignore invalid characters
        return True
    
      
    def checkdata(self):
        ''' Check on received message (The data is at least 1 character long)
        '''
        length = len(self.receivemsg)
        # check on complete message
        if ord(self.receivemsg[0]) > 127: #illegal length, spurious received character
            self.receivemsg = self.receivemsg[1:]
            return False
        if ord(self.receivemsg[length-1]) != CR:
            return False
        else:
            ''' This is end of msg '''
            if self.dspfunction != None:
                if  length > 2:
                    self.dspfunction(self.receivemsg, Recv=self.receivemsg)
            else:
                print('Serial-Received: %s' % (self.receivemsg))
            self.receivemsg = ''
            
        return True

