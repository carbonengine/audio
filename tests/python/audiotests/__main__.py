import unittest
import blue
import stackless


def main():
    import sys

    if sys.argv[0].endswith("__main__.py"):
        import os.path
        # We change sys.argv[0] to make help message more useful
        # use executable without path, unquoted
        # (it's just a hint anyway)
        # (if you have spaces in your executable you get what you deserve!)
        executable = os.path.basename(sys.executable)
        sys.argv[0] = executable + " -m unittest"
        del os

    class TaskletTestRunner(unittest.TextTestRunner):
        def __init__(self, *args, **kwargs):
            self.result = None
            super(TaskletTestRunner, self).__init__(*args, **kwargs)

        def run(self, test):
            stackless.tasklet(self._run_impl)(test)
            while self.result is None:
                blue.os.Pump()

            return self.result

        def _run_impl(self, test):
            self.result = super(TaskletTestRunner, self).run(test)

    unittest.main(module=None, testRunner=TaskletTestRunner())


main()
