import os
import sys

import blue
import unittest
import uthread2


from teamcity import is_running_under_teamcity
from teamcity.unittestpy import TeamcityTestRunner


def run_tests(test_cases):
    global test_result
    if is_running_under_teamcity():
        runner = TeamcityTestRunner(stream=sys.stdout)
    else:
        runner = unittest.TextTestRunner(verbosity=2, stream=sys.stdout)

    suite = unittest.TestSuite(test_cases)
    test_result = runner.run(suite)


if __name__ == "__main__":
    test_loader = unittest.TestLoader()
    test_cases = test_loader.discover(os.getcwd()) 
    tasklet = uthread2.StartTasklet(run_tests, test_cases)
    while tasklet.is_alive():
        blue.os.Pump()

    if not test_result.wasSuccessful():
        sys.exit(1)
