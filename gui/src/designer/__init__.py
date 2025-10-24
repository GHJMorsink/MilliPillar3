''' gui.designer: Interface to GUI designs made with QT Designer.

This module automatically compiles QT Designs to python code, and imports them.
'''
import subprocess
import os
from os import path
import sys


#PYUIC = 'pyside-uic -i 2' if 'linux' in sys.platform else 'pyside-uic.bat -i 2'
#PYRCC = 'pyside-rcc' if 'linux' in sys.platform else 'pyside-rcc.exe'
PYUIC = 'pyuic5 -i 4' if 'linux' in sys.platform else 'pyuic5.exe'
PYRCC = 'pyrcc5' if 'linux' in sys.platform else 'pyrcc5.exe'

def compile_file_ui(filename, dest, compiler):
    ''' Compile a file generate by QT Designer, using a specific compiler.
        The function determines if a file needs compilation using Make-like rules.
    '''
    mtime = path.getmtime(filename)
    if not path.exists(dest) or mtime > path.getmtime(dest):
        print('compiling:', filename, dest, compiler)
        line = ' '.join(['# pylint:',
                    'disable=W0311,C0301,C0111,W0201,R0201,C0302,W0612,W0613,R0902,R0915,'
                    'F0401,E1101\n'])
        line2 = '\nimport designer.%s_rc\n' % path.splitext(path.basename(dest))[0]

        f = open(dest,'w')
        subprocess.call(compiler.split() + [filename], stdout=f, stderr=f)
        f.close()
        fulltext = open(dest).read()
        posimport = fulltext.rfind('import')
        script = line + fulltext[:posimport] + line2
        open(dest, 'w').write(script)

def compile_file_rc(filename, dest, compiler):
    ''' Compile a file generate by QT Designer, using a specific compiler.
        The function determines if a file needs compilation using Make-like rules.
    '''
    mtime = path.getmtime(filename)
    if not path.exists(dest) or mtime > path.getmtime(dest):
        print('compiling:', filename, dest, compiler)
        line = ' '.join(['# pylint:',
                    'disable=W0311,C0301,C0111,W0201,R0201,C0302,W0612,W0613,R0902,R0915,'
                    'F0401,E1101\n'])
        f = open(dest,'w')
        subprocess.call(compiler.split() + [filename], stdout=f, stderr=f)
        f.close()
        script = line + open(dest).read()
        open(dest, 'w').write(script)

def compile_ui(uifilename):
    ''' Compiler front-end for UI files.
    '''
    dest = '.'.join([os.path.splitext(uifilename)[0], 'py'])
    return compile_file_ui(uifilename, dest, PYUIC)
  
def compile_rc(rcfilename):
    ''' Compiler front-end for RC files.
    '''
    dest = '.'.join([os.path.splitext(rcfilename)[0]+'_rc', 'py'])
    return compile_file_rc(rcfilename, dest, PYRCC)

def run():
    ''' This function searches for QT-designer files in the directory where
        this module is located, and subsequently compiles them. 
    '''
    # Compile the UI file
    # only if running from source. Not if running from exe
    #if not py2exe:
    if not py_inst:
        dir_ = os.path.dirname(__file__)
        os.environ['PYTHONPATH'] = os.pathsep.join([dir_,
                                                os.environ.get('PYTHONPATH', '')])
        sys.path = [dir_] + sys.path
        ui_files = [f for f in os.listdir(dir_) if path.splitext(f)[1] == '.ui']
        uifilenames = [path.join(dir_, n) for n in ui_files]
        rc_files = [f for f in os.listdir(dir_) if path.splitext(f)[1] == '.qrc']
        rcfilenames = [path.join(dir_, n) for n in rc_files]
    
        for uifilename in uifilenames:
            compile_ui(uifilename)
        for rcfilename in rcfilenames:
            compile_rc(rcfilename)

# Check if we're running from within the py2exe generated exe
#in python3 __loader__ is standard included
#for PyInstaller:
if getattr(sys, 'frozen', False) and hasattr(sys, '_MEIPASS'):
    #print('running in a PyInstaller bundle')
    py_inst = True
else:
    #print('running in a normal Python process')
    py_inst = False
    
if '__loader__' in dir():
    py2exe = True
else:
    py2exe = False

run()
