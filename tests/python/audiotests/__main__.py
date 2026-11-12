# Copyright © 2024 CCP ehf.

import os
import sys
import unittest

import blue
import scheduler


def main():
    if sys.argv[0].endswith("__main__.py"):
        import os.path
        executable = os.path.basename(sys.executable)
        sys.argv[0] = executable + " -m unittest"
        del os

tasklet_start = lambda func, *args: scheduler.tasklet(func)(*args)

class TaskletTestRunner(unittest.TextTestRunner):
    def __init__(self, *args, **kwargs):
        self.result = None
        super().__init__(*args, **kwargs)

    def run(self, test):
        tasklet_start(self._run_impl, test)
        # Required to avoid sleep when all tasklets yield
        blue.os.sleeptime = 0
        while self.result is None:
            blue.os.Pump()
        return self.result

    def _run_impl(self, test):
        self.result = super().run(test)

def check_audio2_import():
    """Check if audio2 can be imported"""
    try:
        # Test import to catch DLL version conflicts early
        blue.LoadExtension("_audio2")
    except ImportError as e:
        error_msg = str(e)
        buildflavor = os.environ.get('BUILDFLAVOR', 'NOT_SET')
        pythonpath = os.environ.get('PYTHONPATH', 'NOT_SET')

        print("ERROR: Loading audio2 failed!\n"
              "This usually means stale build artifacts are causing DLL conflicts.\n"
              "To fix this:\n"
              "  Step 1: Use VS Code task 'Clean Build Directory' or manually delete the 'out' directory in this repo\n"
              "  Step 2: Re-run your test\n"
              "Original error: {2}".format(buildflavor, pythonpath, error_msg), file=sys.stderr)
        sys.exit(1)

def apply_job_worker_override():
    """Run every test with Wwise job worker threads when AUDIOTESTS_JOB_WORKER_THREADS is set."""
    threads = int(os.environ.get("AUDIOTESTS_JOB_WORKER_THREADS", "0"))
    if threads <= 0:
        return

    import audio2
    from audio2.audiomanager import AudioManager

    # Tests build their settings either straight from audio2.AudSettings or through the AudioManager wrapper.
    create_settings = audio2.AudSettings

    def create_settings_with_job_workers():
        settings = create_settings()
        settings.jobWorkerThreads = threads
        return settings

    audio2.AudSettings = create_settings_with_job_workers

    create_audio_settings = AudioManager._CreateAudioSettings

    def create_audio_settings_with_job_workers(self, *args, **kwargs):
        settings = create_audio_settings(self, *args, **kwargs)
        settings.jobWorkerThreads = threads
        return settings

    AudioManager._CreateAudioSettings = create_audio_settings_with_job_workers

if __name__ == "__main__":
    check_audio2_import()
    apply_job_worker_override()
    unittest.main(module=None, testRunner=TaskletTestRunner())
    main()