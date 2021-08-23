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
import sys

# This is a hack to allow PyCharm to parse stub files for _audio2. The _audio2_stub stub is located
# in packages/stubgen/stubs and will always generate an ImportError.
try:
    from _audio2_stub import *
except ImportError:
    pass

_audio2 = blue.LoadExtension("_audio2")

# We do the work in a function, to keep things out of the main module namespace
def __RegisterEnums( namespace ):
    # You can't add attributes to a pure 'object'
    class enumWrapper( object ):
        pass

    for enum in _audio2.GetRegisteredEnums():
        namespace[enum] = enumWrapper()
        namespace[enum].values = {}
        for enumValueName, value, docString in _audio2.GetRegisteredEnumValues( enum ):
            #namespace[enumValueName] = value                                # Define helloworld.ENUM_VALUE
            setattr( namespace[enum], enumValueName, value )                # Define helloworld.ENUM.ENUM_VALUE
            namespace[enum].values[ enumValueName ] = ( value, docString )  # Define helloworld.ENUM.values[ ENUM_VALUE ]
__RegisterEnums( globals() )


sys.modules[__name__] = _audio2
