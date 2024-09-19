import unittest
import blue
import scheduler


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
            scheduler.tasklet(self._run_impl)(test)
            while self.result is None:
                blue.os.Pump()

            if len(self.result.failures) > 0:
                for failure in self.result.failures:
                    print(failure[1])

            if len(self.result.errors) > 0:
                for error in self.result.errors:
                    print(error, file=sys.stderr)

            return self.result

        def _run_impl(self, test):
            #blue.pyos.synchro.SleepWallclock(8000)
            self.result = super(TaskletTestRunner, self).run(test)

    unittest.main(module=None, testRunner=TaskletTestRunner())


main()
