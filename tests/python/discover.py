import sys
import unittest
import traceback

def print_suite(suite):
    if hasattr(suite, '_exception'):
        print(suite._exception)
        sys.exit(1)
    else:
        if not hasattr(suite, '__iter__'):
            print(suite.id())
        else:
            for s in suite:
                print_suite(s)

if __name__ == '__main__':

    try:
        suite = unittest.defaultTestLoader.discover('.')
        print_suite(suite)
    except Exception as e:
        print("Test discovery error: {}".format(e))
        traceback.print_exc()
        sys.exit(1)