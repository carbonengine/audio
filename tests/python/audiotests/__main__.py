from __future__ import print_function
import os
import sys
import unittest

import blue
PY3 = sys.version_info[0] >= 3


def main():
    if sys.argv[0].endswith("__main__.py"):
        import os.path
        executable = os.path.basename(sys.executable)
        sys.argv[0] = executable + " -m unittest"
        del os

if PY3:
    import scheduler
    tasklet_start = lambda func, *args: scheduler.tasklet(func)(*args)
else:
    import uthread2
    tasklet_start = uthread2.start_tasklet

class TaskletTestRunner(unittest.TextTestRunner):
    def __init__(self, *args, **kwargs):
        self.result = None
        if PY3:
            super().__init__(*args, **kwargs)
        else:
            super(TaskletTestRunner, self).__init__(*args, **kwargs)

    def run(self, test):
        tasklet_start(self._run_impl, test)
        # Required to avoid sleep when all tasklets yield
        blue.os.sleeptime = 0
        while self.result is None:
            blue.os.Pump()
        return self.result

    def _run_impl(self, test):
        if PY3:
            self.result = super().run(test)
        else:
            self.result = super(TaskletTestRunner, self).run(test)

def check_audio2_import():
    """Check if audio2 can be imported"""
    try:
        # Test import to catch DLL version conflicts early
        blue.LoadExtension("_audio2")
    except ImportError as e:
        error_msg = str(e)
        branch_path = os.environ.get('CCP_EVE_PERFORCE_BRANCH_PATH', '[BRANCH_PATH]')
        buildflavor = os.environ.get('BUILDFLAVOR', 'NOT_SET')
        pythonpath = os.environ.get('PYTHONPATH', 'NOT_SET')
        
        print("ERROR: Loading audio2 failed!\n"
              "This usually means stale build artifacts are causing DLL conflicts.\n"
              "To fix this:\n"
              "  Step 1: Use VS Code task 'Clean Build Directory' or manually delete the 'out' directory in this repo\n"
              "  Step 2: Re-run your test\n"
              "Original error: {2}".format(buildflavor, pythonpath, error_msg), file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    check_audio2_import()
    unittest.main(module=None, testRunner=TaskletTestRunner())
    main()