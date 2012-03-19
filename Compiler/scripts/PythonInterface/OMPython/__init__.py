# -*- coding: cp1252 -*-
"""
  This file is part of OpenModelica.
 
  Copyright (c) 1998-CurrentYear, Open Source Modelica Consortium (OSMC),
  c/o Linköpings universitet, Department of Computer and Information Science,
  SE-58183 Linköping, Sweden.
 
  All rights reserved.
 
  THIS PROGRAM IS PROVIDED UNDER THE TERMS OF GPL VERSION 3 LICENSE OR 
  THIS OSMC PUBLIC LICENSE (OSMC-PL) VERSION 1.2. 
  ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS PROGRAM CONSTITUTES RECIPIENT'S ACCEPTANCE
  OF THE OSMC PUBLIC LICENSE OR THE GPL VERSION 3, ACCORDING TO RECIPIENTS CHOICE. 
 
  The OpenModelica software and the Open Source Modelica
  Consortium (OSMC) Public License (OSMC-PL) are obtained
  from OSMC, either from the above address,
  from the URLs: http://www.ida.liu.se/projects/OpenModelica or  
  http://www.openmodelica.org, and in the OpenModelica distribution. 
  GNU version 3 is obtained from: http://www.gnu.org/copyleft/gpl.html.
 
  This program is distributed WITHOUT ANY WARRANTY; without
  even the implied warranty of  MERCHANTABILITY or FITNESS
  FOR A PARTICULAR PURPOSE, EXCEPT AS EXPRESSLY SET FORTH
  IN THE BY RECIPIENT SELECTED SUBSIDIARY LICENSE CONDITIONS OF OSMC-PL.
 
  See the full OSMC Public License conditions for more details.

  Author : Anand Kalaiarasi Ganeson, ganan642@student.liu.se, 2012-03-19
  Version: 1.0
"""
 
import sys
import os
import time
import inspect

if sys.platform == 'win32':
  omhome = os.environ['OPENMODELICAHOME']
  # add OPENMODELICAHOME\lib to PYTHONPATH so python can load omniORB libraries
  sys.path.append(os.path.join(omhome, 'share', 'omc', 'scripts', 'PythonInterface', 'stubs'))
  sys.path.append(os.path.join(omhome, 'lib'))
  # add OPENMODELICAHOME\bin to path so python can find the omniORB binaries
  pathVar = os.getenv('PATH')
  pathVar += ';'
  pathVar += os.path.join(omhome, 'bin')
  os.putenv('PATH', pathVar)
else:
  import OMConfig
  omhome = OMConfig.DEFAULT_OPENMODELICAHOME
  # add OPENMODELICAHOME\lib to PYTHONPATH so python can load omniORB libraries
  sys.path.append(os.path.join(OMConfig.DEFAULT_OPENMODELICAHOME, 'share', 'omc', 'scripts', 'PythonInterface', 'stubs'))

from subprocess import Popen, PIPE
from collections import OrderedDict
from datetime import datetime
from omniORB import CORBA

# import the skeletons for the global module
import _GlobalIDL

# import the parser module
import OMParser

# Randomize the IOR file name
random_string = str(datetime.now())
random_string = ''.join(e for e in random_string if e.isalnum())

# Run the server
ompath = os.path.join(omhome, 'bin', 'omc') + " +d=interactiveCorba" + " +c=" + random_string
server = Popen(ompath, shell=True, stdout=PIPE).stdout

# Locating and using the IOR
import tempfile
temp = tempfile.gettempdir()
if sys.platform == 'win32':
  ior_file = "openmodelica.objid." + random_string
else:
  currentUser = os.environ['USER']
  if currentUser == '':
    currentUser = "nobody"
  ior_file = "openmodelica." + currentUser + ".objid." + random_string
ior_file = os.path.join(temp, ior_file)
omc_corba_uri= "file:///" + ior_file

# See if the omc server is running
if os.path.isfile(ior_file):
  print "OMC Server is up and running at " + omc_corba_uri + "\n"
else:
  attempts = 0
  while True:
    if not os.path.isfile(ior_file):
      time.sleep(0.25)
      attempts +=1
      if attempts == 10:
        print "OMC Server is down. Please start it! Exiting...\n"
        sys.exit(2)
    else:
      print "OMC Server is up and running at " + omc_corba_uri + "\n"
      break

#initialize the ORB with maximum size for the ORB set
sys.argv.append("-ORBgiopMaxMsgSize")
sys.argv.append("2147483647")
orb = CORBA.ORB_init(sys.argv, CORBA.ORB_ID)

# Read the IOR file
objid_file=open(ior_file)
ior = objid_file.readline()
objid_file.close()

# Find the root POA
poa = orb.resolve_initial_references("RootPOA")

# Convert the IOR into an object reference
obj = orb.string_to_object(ior)

# Narrow the reference to the OmcCommunication object
omc = obj._narrow(_GlobalIDL.OmcCommunication)

# Check if we are using the right object
if omc is None:
        print "Object reference is not valid"
        sys.exit(1)

# Helper class to retrieve the results of (nested) dictionaries using dot separated queries
class dotdictify(dict):
    def __init__(self, value=None):
        if value is None:
            pass
        elif isinstance(value, dict):
            for key in value:
                self.__setitem__(key, value[key])
        else:
            raise TypeError, 'expected a dictionary, re-try'

    def __setitem__(self, key, value):
        if '.' in key:
            myKey, restOfKey = key.split('.', 1)
            target = self.setdefault(myKey, dotdictify())
            if not isinstance(target, dotdictify):
                raise KeyError, 'cannot set "%s" in "%s" (%s)' % (restOfKey, myKey, repr(target))
            target[restOfKey] = value
        else:
            if isinstance(value, dict) and not isinstance(value, dotdictify):
                value = dotdictify(value)
            dict.__setitem__(self, key, value)

    def __getitem__(self, key):
        if '.' not in key:
            return dict.__getitem__(self, key)
        myKey, restOfKey = key.split('.', 1)
        target = dict.__getitem__(self, myKey)
        if not isinstance(target, dotdictify):
            raise KeyError, 'cannot get "%s" in "%s" (%s)' % (restOfKey, myKey, repr(target))
        return target[restOfKey]

    def __contains__(self, key):
        if '.' not in key:
            return dict.__contains__(self, key)
        myKey, restOfKey = key.split('.', 1)
        target = dict.__getitem__(self, myKey)
        if not isinstance(target, dotdictify):
            return False
        return restOfKey in target

    def setdefault(self, key, default):
        if key not in self:
            self[key] = default
        return self[key]

    __setattr__ = __setitem__
    __getattr__ = __getitem__

def typeCast(string):
    if string.__class__ == dict:
      string = dict(string)
    elif string.__class__ == list:
      string = list(string)
    elif string.__class__ == float:
      string = float(string)
    elif string.__class__ == long:
      string = long(string)
    elif string.__class__ == bool:
      string = bool(string)
    elif string.__class__ == tuple:
      string = tuple(string)
    elif string.__class__ == complex:
      string = complex(string)
    elif string.__class__ == int:
      string = int(string)
    elif string.__class__ == file:
      string = file(string)
    elif string.__class__ == str:
      string = str(string)
    elif inspect.isclass(dotdictify):
      string = dict(string)
      if string.__class__ == dict:  
        string = dict(string)
    else:
      print "Unknown datatype :: %s"% string
    return string

def get(root,query):  
  if isinstance(root,dict):
    root = dotdictify(root)
      
  try:
    result = root[query]
    result = typeCast(result)
    return result
  except KeyError:
    print "KeyError: Cannot GET the value, please check the syntax of your dotted notationed query"

def set(root,query,value):
  if isinstance(root,dict):
    root = dotdictify(root)
      
  try:
    root[query]=value
    result = typeCast(root)
    return result
  except KeyError:
    print "KeyError: Cannot SET the value, please check your dotted notationed query"

# Invoke the sendExpression method to send text commands to the server
def send_command(command):
        if command == "quit()":
                omc.sendExpression("quit()")
                print "OMC has been Shutdown\n"
                sys.exit(1)
        else:
                result = omc.sendExpression(command)
                if result[0] == "\"":
                        return result
                else:
                        answer = OMParser.check_for_values(result)
                        OMParser.result = {}
                        return answer

# Test commmands        
def run():
        omc_running = True
        while omc_running:
                command = raw_input("\n>>")
                if command == "quit()":
                        omc.sendExpression("quit()")
                        print "OMC has been Shutdown\n"
                        omc_running = False
                        sys.exit(1)
                else:
                        result = omc.sendExpression(command)
                        if result[0] == "\"":
                                print result
                        else:
                                answer = OMParser.check_for_values(result)
                                OMParser.result = {}
                                print answer

if __name__ == "__main__":
        run()
