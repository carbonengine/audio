#   *************************************************************************
#
#   __init__.py
#
#   Created by: Andri Mar
#   Created on: 10-Oct-2008 as audio2.py
#   Moved on 13-Jan-2009 to audio2/__init__.py
#
#   Description:
#
#       An indirection point so that we can switch in different flavors
#       of our audio DLLs when people execute 'import audio'.
#
#   (c) CCP 2008
#
#   *************************************************************************

import blue

commdev = 0
mem = 0
args = blue.pyos.GetArg()
for arg in args:
    if arg.startswith("/audiodev"):
        commdev = 1
        break
    if arg == "/audiomem":
        mem = 1
        break

if commdev == 1:
    try:
        from _audio2_dev import *
        print "Audio2 imported from _audio2_dev"
    except ImportError:
        print "Import from _audio2_dev failed - fallback to _audio2"
        from _audio2 import *
elif mem:
    try:
        from _audio2_mem import *
        print "Audio2 imported from _audio2_mem for memory tracking"
    except ImportError:
        print "Import from _audio2_mem failed - fallback to _audio2"
        from _audio2 import *
else:
    from _audio2 import *



# We do the work in a function, to keep things out of the main module namespace
def __RegisterEnums( namespace ):
    # You can't add attributes to a pure 'object'
    class enumWrapper( object ):
        pass

    for enum in GetRegisteredEnums():
        namespace[enum] = enumWrapper()
        namespace[enum].values = {}
        for enumValueName, value, docString in GetRegisteredEnumValues( enum ):
            #namespace[enumValueName] = value                                # Define helloworld.ENUM_VALUE
            setattr( namespace[enum], enumValueName, value )                # Define helloworld.ENUM.ENUM_VALUE
            namespace[enum].values[ enumValueName ] = ( value, docString )  # Define helloworld.ENUM.values[ ENUM_VALUE ]
__RegisterEnums( globals() )

def GetEnumValueName( enumName, value ):
    if enumName in globals():
        enum = globals()[enumName]
        result = ""
        for enumKeyName, (enumKeyValue, enumKeydocString) in enum.values.iteritems():
            if enumKeyValue == value:
                if result != "":
                    result += " | "
                result += enumKeyName
        return result

def GetEnumValueNameAsBitMask( enumName, value ):
    if enumName in globals():
        enum = globals()[enumName]
        result = ""
        for enumKeyName, (enumKeyValue, enumKeydocString) in enum.values.iteritems():
            if (enumKeyValue & value) == enumKeyValue:
                if result != "":
                    result += " | "
                result += enumKeyName
        return result

#Remove vars we don't need or want in Jessicas' autocomplete feature
del commdev
del args
del mem
