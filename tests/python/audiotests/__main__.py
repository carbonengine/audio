import unittest
import blue
import sys
import time

# import scheduler only for Python 3
PY3 = sys.version_info[0] >= 3
if PY3:
    import scheduler

class BaseTestRunner:
    """Common functionality for both Python 2 and 3 test runners"""
    def handle_pump(self):
        blue.os.Pump()
        blue.pyos.synchro.SleepWallclock(10)

def main():
    if sys.argv[0].endswith("__main__.py"):
        import os.path
        executable = os.path.basename(sys.executable)
        sys.argv[0] = executable + " -m unittest"
        del os

    if PY3:
        class TaskletTestRunner(unittest.TextTestRunner, BaseTestRunner):
            def __init__(self, *args, **kwargs):
                self.result = None
                self.timeout = kwargs.pop('timeout', 30.0)
                super(TaskletTestRunner, self).__init__(*args, **kwargs)

            def run(self, test):
                start_time = time.time()
                scheduler.tasklet(self._run_impl)(test)
                
                while self.result is None:
                    if time.time() - start_time > self.timeout:
                        raise TimeoutError("Test execution timed out")
                    self.handle_pump()

                if len(self.result.failures) > 0:
                    for failure in self.result.failures:
                        print(failure[1])

                if len(self.result.errors) > 0:
                    for error in self.result.errors:
                        sys.stderr.write(str(error) + '\n')

                return self.result

            def _run_impl(self, test):
                self.result = super(TaskletTestRunner, self).run(test)

        test_runner = TaskletTestRunner
    else:
        # Use standard test runner for Python 2
        class Py2TestRunner(unittest.TextTestRunner, BaseTestRunner):
            def run(self, test):
                result = super(Py2TestRunner, self).run(test)
                self.handle_pump()
                return result

        test_runner = Py2TestRunner

    unittest.main(module=None, testRunner=test_runner)