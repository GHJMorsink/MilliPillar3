# pylint: disable-msg=C0302
'''Startup gui'''

#pylint: disable-msg=E0611,E1101,E1103,F0401, E1002, W0105
#if the designer block isn't compiled these errors will occur
import os
import time
from PyQt5 import QtGui, QtCore, QtWidgets

from designer.stimulator import Ui_MilliPillarControl


home = os.path.dirname(__file__)


RESISTOR = 100.0        #BASE RESISTOR IN OHM (for calibration)

# calibrations (should go to an array, recalled from device
CalibrationCURRENTNUL = 127
CalibrationCURRENT = 0.156
CalibrationFVNUL = 128
CalibrationFV = 0.040
CalibrationTVNUL = 128
CalibrationTV = 0.041

class MyMainWindow(QtWidgets.QMainWindow):
    ''' classdoc MyMainWindow '''
    def __init__(self, app, methods, parent=None):
        '''  constructor '''
        super().__init__(parent)
        self.stopped = True
        self._app = app
        self.methods = methods
        self.upgradefile = ''
        self.ui = Ui_MilliPillarControl()
        self.ui.setupUi(self)
        #self.ui.versionLabel.setText('USB Curve Tracer  Version 0.1_%s.\n' \
        #                             '\251 2025, GHJ Morsink'
        #                              % __build_version__)
        # Calibration
        self.ui.CalibButton.clicked.connect(self._setCorrFactors)
        # about block ...
        self.ui.browsePushButton.clicked.connect(self._browse)
        self.ui.FlashButton.clicked.connect(self._flash)
        self.ui.pushButton_Recorder.clicked.connect(self._changeRec)
        self.ui.pushButton_Run.clicked.connect(self._changeRun)
        self.ui.pushButton_Store.clicked.connect(self._storeSettings)
        self.ui.TimingT1Edit.editingFinished.connect(self._timingchanged)
        self.ui.TimingT2Edit.editingFinished.connect(self._timingchanged)
        self.ui.TimingT3Edit.editingFinished.connect(self._timingchanged)
        self.ui.TimingT4Edit.editingFinished.connect(self._timingchanged)
        self.ui.comportsBox.activated.connect(self._newcomport)
        
        self.ui.SerialNumberEdit.editingFinished.connect(self._newserial)
        self.show()


    def get_initial_data(self):
        ''' give the commands to get the initial data
           Needs some time at beginning for startup after a reset
           and intermediate for the normal flow of the device.
        '''
        time.sleep(0.5)
        self.methods.sendMsg('eo')  # no echos
        time.sleep(0.05)
        self.methods.sendMsg('ve')  # show firmwareversion
        time.sleep(0.05)
        self.methods.sendMsg('sn')  # show serial number
        time.sleep(0.05)
        self.methods.sendMsg('gs')  # show the status of the run/record
        time.sleep(0.05)
        self.methods.sendMsg('ss')  # show settings
        #The responses will automatically be shown on screen
        

    def _timingchanged(self):
        ''' Text in one of the timing elements is changed; adapt BPM
        '''
        try:
            t1 = float(self.ui.TimingT1Edit.text())
            t2 = float(self.ui.TimingT2Edit.text())
            t3 = float(self.ui.TimingT3Edit.text())
            t4 = float(self.ui.TimingT4Edit.text())
            self.setBPM(t1, t2, t3, t4)
        except:
            pass

    def _newserial(self):
        ''' Serial number is edited; check it is ended with specific character
            and if so, send it
        '''
        req = self.ui.SerialNumberEdit.text()
        if req[-1:] == 'C':
            self.methods.sendMsg('sn %s' % req)
        else:
            self.methods.sendMsg('sn')


    def _newcomport(self):
        ''' a new com port is selected
        '''
        newcom = self.ui.comportsBox.currentText()
        self.methods.setPort(newcom)

                            
    def _changeRec(self):
        ''' change the recording function '''
        if self.ui.pushButton_Recorder.isChecked():
            self.methods.sendMsg('re')
        else:
            self.methods.sendMsg('nr')


    def _changeRun(self):
        ''' change the pulsing '''
        if self.ui.pushButton_Run.isChecked():
            self.methods.sendMsg('ru')
        else:
            self.methods.sendMsg('of')


    def _storeSettings(self):
        ''' store timing and voltage setting '''
        try:
            v1 = int(float(self.ui.FirstVoltageEdit.text()) * 10)
            v2 = -int(float(self.ui.SecondVoltageEdit.text()) * 10)
            t0 = int(self.ui.StarttimeEdit.text())
            t1 = int(float(self.ui.TimingT1Edit.text()) * 10)
            t2 = int(float(self.ui.TimingT2Edit.text()) * 10)
            t3 = int(float(self.ui.TimingT3Edit.text()) * 10)
            t4 = int(self.ui.TimingT4Edit.text())
            self.methods.sendMsg('sv %d,%d' % (v1, v2))
            self.methods.sendMsg('st %d,%d,%d,%d,%d' % (t0, t1, t2, t3, t4))
            self.methods.sendMsg('wr')
        except:
            self.UpdateScrStatus('Wrong data!')


    def _browse(self):
        '''Browse the file system, show a file browser popup (select the hex-file to flash)'''
        fileName = QtWidgets.QFileDialog.getOpenFileName(None,
                                                     "Select Hex file", "", "*.hex")
        if fileName[0] != "":
            self.ui.fileNameLineEdit.setText(fileName[0])


    def _flash(self):
        ''' run the flasher '''
        #print('flash')
        self.methods.doReset(str(self.ui.fileNameLineEdit.text()),
                             str(self.ui.comportsBox.currentText()))


    def _setCorrFactors(self):  # TODO!
        ''' set the calibration data '''
        self.UpdateScrStatus('Work in progress; not implemented yet!')
        #self.methods.calibrate(self.ui.CorrFactorEdit.text())


    def setComlist(self, comlist, preferred):
        ''' Fill the dropdown list of comports, and select preferred
        '''
        for p in comlist:
            self.ui.comportsBox.addItem(p)
        self.ui.comportsBox.setCurrentIndex(preferred)
        self.get_initial_data()


    def setBPM(self, time1, time2, time3, time4):
        ''' Set / calculate the beats per minute
        '''
        totaltime = time1+time2+time3+time4     # in ms
        bpm = 60000/totaltime
        self.ui.BPMEdit.setText('%.1f' % bpm)


    def readCurrent(self, hexstr, viewElement):
        ''' Make current out of received hex value
        '''
        if hexstr == 'FF':
            viewElement.setText('--')
        else:
            raw = int(hexstr, 16) - CalibrationCURRENTNUL
            raw = raw * CalibrationCURRENT
            viewElement.setText('%.2f' % raw)
        
    def readFullVoltage(self, hexstr, viewElement):
        ''' Make voltage out of received hex value
        '''
        if hexstr == 'FF':
            viewElement.setText('--')
        else:
            raw = int(hexstr, 16) - CalibrationFVNUL
            raw = raw * CalibrationFV
            viewElement.setText('%.2f' % raw)
        
    def readTissVoltage(self, hexstr, viewElement):
        ''' Make voltage out of received hex value
        '''
        if hexstr == 'FF':
            viewElement.setText('--')
        else:
            raw = int(hexstr, 16) - CalibrationTVNUL
            raw = raw * CalibrationTV
            viewElement.setText('%.2f' % raw)


    def cleanMeasured(self):
        ''' Set all measured values to invalid
        '''
        self.ui.TissueAFirst.setText('--')
        self.ui.MeasuredFirstEdit.setText('--')
        self.ui.TissueVFirst.setText('--')
        self.ui.TissueASecond.setText('--')
        self.ui.MeasuredSecondEdit.setText('--')
        self.ui.TissueVSecond.setText('--')
        self.ui.TissueRestT2.setText('--')
        self.ui.TissueRestT4.setText('--')

        
    #pylint: disable=R0913
    def UpdateScrStatus(self, text, Recv = None):
        ''' Change values on screen in status and progress '''
        self.ui.statusbar.showMessage(text)
        if Recv:
            Recv = Recv.strip()     # remove newlines/CR at front and back
            if Recv.find('TERM> ') == 0 :
                Recv = Recv[6:]
            if Recv.find('Stimulator Version') == 0 :
                self.ui.FirmwareVersion.setText(Recv[19:])
            elif Recv.find('Serial:') >= 0:
                self.ui.SerialNumberEdit.setText(Recv[7:])
            elif Recv.find('!:') == 0:      # button status for recording and running
                if Recv[3:5] == '00':
                    self.ui.pushButton_Recorder.setChecked(False)
                else:
                    self.ui.pushButton_Recorder.setChecked(True)
                if Recv[6:8] == '00':
                    self.ui.pushButton_Run.setChecked(False)
                    self.cleanMeasured()
                else:
                    self.ui.pushButton_Run.setChecked(True)
            elif Recv.find('Voltage V1, V2:') == 0:
                Recv = Recv[16:].strip()
                Comma = Recv.find(',')
                Vpos = eval(Recv[:Comma])/10
                self.ui.FirstVoltageEdit.setText('%.2f' % Vpos) 
                Vneg = eval(Recv[(Comma+2):])/10
                self.ui.SecondVoltageEdit.setText('-%.2f' % Vneg)
            elif Recv.find('Timings T0,T1,T2,T3,T4:') == 0:
                Recv = Recv[23:].strip() 
                Comma = Recv.find(',')
                time0 = int(Recv[:Comma])
                Recv = Recv[Comma+1:]
                Comma = Recv.find(',')
                time1 = int(Recv[:Comma])/10
                Recv = Recv[Comma+1:]
                Comma = Recv.find(',')
                time2 = int(Recv[:Comma])/10
                Recv = Recv[Comma+1:]
                Comma = Recv.find(',')
                time3 = int(Recv[:Comma])/10
                time4 = int(Recv[Comma+1:])
                self.ui.StarttimeEdit.setText('%d' % time0)
                self.ui.TimingT1Edit.setText('%.1f' % time1)
                self.ui.TimingT2Edit.setText('%.1f' % time2)
                self.ui.TimingT3Edit.setText('%.1f' % time3)
                self.ui.TimingT4Edit.setText('%d' % time4)
                self.setBPM(time1, time2, time3, time4)
            elif Recv.find(': ') == 0:
                self.readCurrent(Recv[2:4], self.ui.TissueAFirst)
                self.readFullVoltage(Recv[5:7], self.ui.MeasuredFirstEdit)
                self.readTissVoltage(Recv[8:10], self.ui.TissueVFirst)
                #cur2 = self.readCurrent(Recv[11:13])
                #vol2 = self.readFullVoltage(Recv[14:16])
                self.readTissVoltage(Recv[17:19], self.ui.TissueRestT2)
                self.readCurrent(Recv[20:22], self.ui.TissueASecond)
                self.readFullVoltage(Recv[23:25], self.ui.MeasuredSecondEdit)
                self.readTissVoltage(Recv[26:28], self.ui.TissueVSecond)
                #cur2 = self.readCurrent(Recv[29:31])
                #vol2 = self.readFullVoltage(Recv[32:34])
                self.readTissVoltage(Recv[35:37], self.ui.TissueRestT4)
        self._app.processEvents()
        return True


    def show(self):
        ''' Start the constant timer '''
        QtWidgets.QMainWindow.show(self)



class GuiBuilder(object):
    ''' GuiBuilder '''

    def __init__(self, methods ):
        '''Constructor'''
        self.app = QtWidgets.QApplication([])
        self.mw = MyMainWindow(self.app, methods)
        self.app.aboutToQuit.connect(self.exitHandler)
        self.app.lastWindowClosed.connect(self.exitHandler)


    def exitHandler(self):
        ''' system stops '''
        self.mw.methods.stop()


    def start(self):
        '''Run the gui event loop'''
        # self.mw.setWindowTitle("MilliPillar Stimulator V3")
        self.mw.show()
        self.app.setStyle(QtWidgets.QStyleFactory.create('Windows'))
        # self.app.setStyle(QtWidgets.QStyleFactory.create('Plastique'))
        self.app.exec_()
