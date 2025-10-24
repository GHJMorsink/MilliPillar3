'''
Main program startup for the programmer part
--------------------------------------

Contains the main configuration startup and starts all elements
Normal function:
  - Read the hex file
  - Reset connected arduino
  - ping every 0.03 s for reaction
  - if within 5 sec reaction:
    - Repeat:
       - Transfer datablock 128 bytes, and 
       - Flash
    - Until ready
    - Send reset 
    
    Note: timing is delicate
'''

import sys
import time
import optparse
import traceback
import serial

from loadhex import HexFile


BLOCKSIZE = 128     # size for the ATmega328

# STK500 Commands

STK_OK              = b'\x10'  #0x10
STK_INSYNC          = b'\x14'  #0x14  #// ' '
CRC_EOP             = 0x20  #// 'SPACE'
STK_GET_SYNC        = 0x30  #// '0'
STK_GET_SIGN_ON     = 0x31  #// '1'
STK_ENTER_PROGMODE  = 0x50  #// 'P'
STK_LEAVE_PROGMODE  = 0x51  #// 'Q'
STK_LOAD_ADDRESS    = 0x55  #// 'U'
STK_PROG_PAGE       = 0x64  #// 'd'


class Bootload(object):
    ''' Contains the  application '''
    def __init__(self, hexfile, comport):
        ''' Application (root) object constructor '''
        self.pagesize = 0
        self.flashsize = 0
        self.datafile = HexFile(hexfile)
        self.hexfile = hexfile
        self.dspfunction = None
        self.serial = None
        retry = 0
        while retry < 2 and self.serial == None:
            try:
                self.serial = serial.Serial( comport,
                                             115200,
                                             parity='N',
                                             rtscts=False, xonxoff=False, timeout=0 )
            except:
                time.sleep(1)
                retry += 1
        if self.serial.isOpen():
            self.serial.close()
        try:
            self.serial.open()
        except:
            print(traceback.format_exc())
        self.serial.dtr = False
        self.serial.rts = False


    def stop(self):
        ''' Stopping this thread '''
        self.serial.dtr = False
        self.serial.rts = False
        self.serial.close()


    def sendCommand(self, cmd, data):
        ''' Send a specific command to the device '''
        fullcommand = [cmd]
        if data:
            fullcommand += data
        fullcommand += [CRC_EOP]
        if self.serial != None:
            self.serial.write(fullcommand)
            

    def waitforChar(self, char):
        ''' wait for the INSYNC / STK_OK'''
        count = 0
        while count < 5:
            recv = self.serial.read(1)
            #if recv != '':
            #    print('recv %r' % recv)
            if recv == char:
                return True
            time.sleep(0.01)
            count += 1
        return False
    

    def _pingpresence(self):
        ''' Check for presence of the Device in programming mode
            If it reacts on the message: the Device is in boot programming mode
        '''
        counter = 0
        while counter < 5: # maximal 0.5 seconds pinging
            self.sendCommand(STK_GET_SYNC, None)
            time.sleep(0.01)               # wait for sending it
            if self.waitforChar(STK_INSYNC):
                time.sleep(0.05)
                if self.waitforChar(STK_OK):
                    return True
            time.sleep(0.005)  # wait 10 ms
            counter += 1
        return False  # nothing found


    def _sendblock(self, blk):
        ''' Send a block of maximal BLOCKSIZE bytes to the device (and flash) '''
        flashstartaddress = blk * BLOCKSIZE   # this gives the  address
        flashwordaddress = (flashstartaddress >> 1)
        internalbuf = self.datafile.getmemoryportion(flashstartaddress, BLOCKSIZE)
        flashlength = len(internalbuf)
        counter = 0
        while counter < 3:
            counter += 1
            addr = [flashwordaddress & 0xFF, (flashwordaddress >> 8) & 0xFF]
            self.sendCommand(STK_LOAD_ADDRESS, addr)
            time.sleep(0.01)    # 4 bytes at high speed to send
            if self.waitforChar(STK_INSYNC):
                time.sleep(0.01)
                if self.waitforChar(STK_OK):
                    data = [0, flashlength, 1] + internalbuf
                    self.sendCommand(STK_PROG_PAGE, data)
                    time.sleep(flashlength/1000)    # wait for emitting the data
                    if self.waitforChar(STK_INSYNC):
                        time.sleep(0.1) # flash it
                        if self.waitforChar(STK_OK):
                            return True
                    print('Error sending block %d' % blk)
            time.sleep(0.1)
        return False


    def start_cl(self, displayfunction = None):
        ''' Commandline interface '''
        self.dspfunction = displayfunction
        if self.dspfunction:
            self.dspfunction( "Loading %s" % self.hexfile, Recv=None )
        length = self.datafile.readfile()
        maxblocknumber = length / BLOCKSIZE
        block = 0
        self.serial.dtr = True   # give a hardware reset!
        self.serial.rts = True   # give a hardware reset!
        time.sleep(0.4)                     # wait 500 ms
        if not self._pingpresence():
            if self.dspfunction:
                self.dspfunction("Connection to device not present")
            self.stop()         # release the connection (A must!)
            return 1
        #connected
        #print "Flashing"
        while block <= maxblocknumber:
            if self.dspfunction:
                self.dspfunction("%3d%%,  Flashaddress %04X" % ((100*block)/maxblocknumber, (block * BLOCKSIZE)))
            if not self._sendblock(block):
                if self.dspfunction:
                    self.dspfunction("Flashing not succeeded")
                self.stop()         # release the connection (A must!)
                return 1
            block += 1
        if self.dspfunction:
            self.dspfunction("Flashed SUCCESSFUL")
        #print("Successful")
        self.sendCommand(STK_LEAVE_PROGMODE, None)
        # We do not wait for verification, just leave the bootloading!
        self.stop()         # release the connection (A must!)
        time.sleep(0.5) # show message
        return 0


# Test and independent actions
def main():
    ''' Starts the application '''
    parser = optparse.OptionParser(
        usage = "%prog [options] [hexfile]",
        description = "AVR-flasher for python-supported systems (console app)" )

    parser.add_option("-f", "--file", dest = "hexfile",
                    help = "The Intel-hex file to be flashed (without the "'".hex"'")",
                    default = '')
    parser.add_option("-p", "--port", dest = "port",
                      help = "Serial port for programming",
                      default = "COM1")

    orgargs = sys.argv
    (options, args) = parser.parse_args()

    if len(orgargs) == 1:
        parser.print_help()
        sys.exit(1)

    if len(orgargs) > 1:
        if (args == []) and (options.hexfile == ''):
            parser.print_help()
            sys.exit(1)
        if options.hexfile == '':
            options.hexfile = args[0]

    application = Bootload(options.hexfile, options.port)
    if application.start_cl() != 0:
        sys.exit(1)


if __name__ == '__main__':
    main()
