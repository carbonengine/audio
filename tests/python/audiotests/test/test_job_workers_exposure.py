# Copyright © 2026 CCP ehf.

import os
import unittest

from audiotests.base_test_class import COMMON_BNK, LOOP_BNK, LOOP_EVENT, ONE_SHOT_BNK, ONE_SHOT_EVENT
from audiotests.base_test_class import BaseAudio2TestClass
from audiotests.utils import PumpOSWithTimeout, WaitForSoundBanksToLoad

JOB_WORKER_OVERRIDE = "AUDIOTESTS_JOB_WORKER_THREADS"
MAX_JOB_WORKER_THREADS = 8
EMITTER_COUNT = 8


def ExpectedThreads(requested):
    """The thread count Carbon Audio should settle on: capped at 8 and at one less than the logical cores."""
    return max(0, min(requested, MAX_JOB_WORKER_THREADS, (os.cpu_count() or 1) - 1))


def OverrideThreads():
    """The thread count the test runner forces on every test, 0 when it forces none."""
    return max(0, int(os.environ.get(JOB_WORKER_OVERRIDE, "0")))


class JobWorkerTestClass(BaseAudio2TestClass):
    """Starts audio with JOB_WORKER_THREADS job workers, or with whatever the process already uses when it is None.

    The threads are created when Wwise initializes, which happens once per process, so a class that asks for a
    specific count has to be the first to start audio. ctest runs every test in a process of its own.
    """
    JOB_WORKER_THREADS = None

    @classmethod
    def setUpClass(cls):
        from audio2.audiomanager import AUDIO_STATE_UNINITIALIZED
        super().setUpClass()
        if cls.JOB_WORKER_THREADS is not None:
            if cls.audioManager.GetState() != AUDIO_STATE_UNINITIALIZED:
                raise unittest.SkipTest("Wwise is already initialized in this process, the job worker setting can no longer be applied.")
            cls.audioManager.settings.jobWorkerThreads = cls.JOB_WORKER_THREADS
            cls.audioManager.manager.UpdateSettings(cls.audioManager.settings)
        cls.Initialize(cls, defaultSoundBanks=[COMMON_BNK, LOOP_BNK, ONE_SHOT_BNK])
        cls.audioManager.DisableSoundPrioritization()

    def setUp(self):
        self.audioManager.Enable()
        self.manager = self.audioManager.manager

    def tearDown(self):
        self.audioManager.Disable()


class TestJobWorkersDefault(JobWorkerTestClass):
    def test_job_workers_are_off_by_default(self):
        """Without the setting all audio stays on the audio thread and the runtime control does nothing."""
        if OverrideThreads() > 0:
            self.skipTest("{} overrides the default.".format(JOB_WORKER_OVERRIDE))

        self.assertEqual(self.audioManager.settings.jobWorkerThreads, 0)
        self.assertEqual(self.manager.jobWorkerThreadCount, 0)
        self.assertEqual(self.manager.jobWorkersActive, 0)
        self.assertEqual(self.manager.jobWorkerRequestsServed, 0)

        self.manager.jobWorkersActive = 2
        self.assertEqual(self.manager.jobWorkersActive, 0)


class TestJobWorkersOverride(JobWorkerTestClass):
    """Guards the switch that runs the whole suite with job workers, so a run with it set cannot quietly test nothing."""

    def overrideThreads(self):
        threads = OverrideThreads()
        if threads == 0:
            self.skipTest("{} is not set.".format(JOB_WORKER_OVERRIDE))
        return threads

    def test_override_reaches_the_audio_manager_wrapper(self):
        threads = self.overrideThreads()
        self.assertEqual(self.audioManager.settings.jobWorkerThreads, threads)
        self.assertEqual(self.manager.jobWorkerThreadCount, ExpectedThreads(threads))

    def test_override_reaches_plain_settings(self):
        import audio2
        threads = self.overrideThreads()
        self.assertEqual(audio2.AudSettings().jobWorkerThreads, threads)


class TestJobWorkersNegative(JobWorkerTestClass):
    JOB_WORKER_THREADS = -1

    def test_negative_job_workers_are_off(self):
        self.assertEqual(self.manager.jobWorkerThreadCount, 0)


class TestJobWorkersCapped(JobWorkerTestClass):
    JOB_WORKER_THREADS = 64

    def test_job_workers_are_capped(self):
        self.assertEqual(self.manager.jobWorkerThreadCount, ExpectedThreads(64))


class TestJobWorkersEnabled(JobWorkerTestClass):
    JOB_WORKER_THREADS = 2

    def setUp(self):
        self.expectedThreads = ExpectedThreads(self.JOB_WORKER_THREADS)
        if self.expectedThreads == 0:
            self.skipTest("This machine has too few logical cores for job workers.")
        super().setUp()

    def anyEmitterPlaying(self):
        return any(len(emitter.GetPlayingEvents()) > 0 for emitter in self.emitters)

    def test_job_worker_thread_count(self):
        self.assertEqual(self.manager.jobWorkerThreadCount, self.expectedThreads)
        self.assertEqual(self.manager.jobWorkersActive, self.expectedThreads)

    def test_job_workers_active_is_clamped(self):
        self.manager.jobWorkersActive = 1
        self.assertEqual(self.manager.jobWorkersActive, 1)

        self.manager.jobWorkersActive = 0
        self.assertEqual(self.manager.jobWorkersActive, 1)

        self.manager.jobWorkersActive = 99
        self.assertEqual(self.manager.jobWorkersActive, self.expectedThreads)

    def test_job_workers_keep_serving_after_disable_and_enable(self):
        """Disable keeps Wwise and its workers running, and they still serve Wwise once audio is enabled again."""
        import audio2
        self.audioManager.Disable()
        self.audioManager.Enable()
        self.assertEqual(self.manager.jobWorkerThreadCount, self.expectedThreads)

        audio2.GetListener().SetPosition((0,0,0), (0,0,0), (0,0,0))
        self.assertTrue(WaitForSoundBanksToLoad([LOOP_EVENT]), "Timed out waiting for the test SoundBanks to load.")
        emitter = audio2.AudEmitter("jobWorkerEmitterAfterEnable")
        emitter.SetPlacement((0,0,0), (0,0,0), (0,0,0))
        servedBefore = self.manager.jobWorkerRequestsServed
        self.assertGreater(emitter.SendEvent(LOOP_EVENT), 0)
        self.assertTrue(
            PumpOSWithTimeout(lambda: self.manager.jobWorkerRequestsServed <= servedBefore, maxTries=50),
            "The job workers served nothing after audio was enabled again."
        )
        emitter.StopAll()

    def test_events_play_and_finish_with_job_workers(self):
        """Events render and their end of event callbacks, which can now come from a worker, still clear the playing events."""
        import audio2
        listener = audio2.GetListener()
        listener.SetPosition((0,0,0), (0,0,0), (0,0,0))
        self.assertTrue(WaitForSoundBanksToLoad([LOOP_EVENT, ONE_SHOT_EVENT]), "Timed out waiting for the test SoundBanks to load.")

        servedBefore = self.manager.jobWorkerRequestsServed
        self.emitters = []
        for index in range(EMITTER_COUNT):
            emitter = audio2.AudEmitter("jobWorkerEmitter{}".format(index))
            emitter.SetPlacement((index,0,0), (0,0,0), (0,0,0))
            self.emitters.append(emitter)

        for emitter in self.emitters:
            self.assertGreater(emitter.SendEvent(ONE_SHOT_EVENT), 0)
        self.assertTrue(PumpOSWithTimeout(self.anyEmitterPlaying, maxTries=100), "One shots never reported their end of event.")

        for emitter in self.emitters:
            self.assertGreater(emitter.SendEvent(LOOP_EVENT), 0)
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=5)
        for emitter in self.emitters:
            self.assertEqual(list(emitter.GetPlayingEvents().values()), [LOOP_EVENT])

        # Wwise only asks for workers when it has jobs to share out, so this is what shows the threads did real work.
        servedWhilePlaying = self.manager.jobWorkerRequestsServed
        self.assertGreater(servedWhilePlaying, servedBefore, "Wwise never handed a job to the worker threads.")

        self.manager.jobWorkersActive = 1
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=5)
        self.assertGreater(self.manager.jobWorkerRequestsServed, servedWhilePlaying, "A single active worker served nothing.")
        self.manager.jobWorkersActive = self.expectedThreads

        for emitter in self.emitters:
            emitter.StopAll()
        self.assertTrue(PumpOSWithTimeout(self.anyEmitterPlaying, maxTries=50), "Stopped loops never reported their end of event.")
