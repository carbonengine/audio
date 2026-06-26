# Copyright © 2024 CCP ehf.

import sys
import unittest
import traceback


# Mock C-extension modules so configure-time discovery works under the stock
# python interpreter (no BUILDFLAVOR-aware exefile loader required).
sys.modules['blue'] = sys


class MockTasklet:
    def __init__(*args, **kwargs):
        pass


class MockTaskletExt:
    def __init__(*args, **kwargs):
        pass


class MockTaskletExit:
    def __init__(*args, **kwargs):
        pass


sys.modules['_scheduler'] = sys
import _scheduler as mod
mod.TaskletExt = MockTaskletExt
mod.TaskletExit = MockTaskletExit
mod.tasklet = MockTasklet
sys.modules['scheduler'] = mod


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
