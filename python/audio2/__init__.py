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

audio2 = blue.LoadExtension("_audio2")

for memberName in dir(audio2):
    globals()[memberName] = getattr(audio2, memberName)
del audio2