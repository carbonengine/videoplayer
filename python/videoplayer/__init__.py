import blue
import sys

# This is a hack to allow PyCharm to parse stub files for _videoplayer. The _videoplayer_stub stub is located
# in packages/stubgen/stubs and will always generate an ImportError.
try:
    from _videoplayer_stub import *
except ImportError:
    pass

videoplayer = blue.LoadExtension("_videoplayer")
for each in dir(videoplayer):
    globals()[each] = getattr(videoplayer, each)

del blue
del sys
del videoplayer
del each
