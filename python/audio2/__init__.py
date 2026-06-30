# Copyright © 2008 CCP ehf.

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
