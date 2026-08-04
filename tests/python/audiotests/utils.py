# Copyright © 2023 CCP ehf.

import blue
import json
import os

AUDIO_METADATA_FILEPATH = os.path.abspath(os.path.join(os.path.dirname(__file__), "test", "soundbanks", "SoundPrioritizationMetadata.json"))
def PumpOSWithTimeout(booleanFunc, maxTries=10):
    numTries = 0
    while( numTries < maxTries ):
        if not booleanFunc():
            return True
        blue.pyos.synchro.SleepWallclock(100)
        blue.os.Pump()
        numTries += 1
    return not booleanFunc()

def WaitForSoundBanksToLoad(events, maxTries=20):
    """Pump until every given event can be posted, which means its SoundBank has finished loading.
    SoundBanks are loaded asynchronously so SendEvent returns 0 until the load callback has been hit.
    """
    import audio2
    probe = audio2.AudEmitter("soundBankLoadProbe")
    probe.SetPlacement((0,0,0), (0,0,0), (0,0,0))
    allLoaded = True
    for event in events:
        if not PumpOSWithTimeout(lambda event=event: probe.SendEvent(event) <= 0, maxTries=maxTries):
            allLoaded = False
    probe.StopAll()
    return allLoaded

def WaitForEmitterToWake(emitter, maxTries=10):
    """Pump until sound prioritization has woken the emitter. Culled emitters queue events instead of posting them."""
    return PumpOSWithTimeout(emitter.IsCulled, maxTries=maxTries)

def GetAudioMetadataFromFile():
    """Gets audio metadata from file and returns it as a dict. Also converts eventIDs to int in the process."""
    with open(AUDIO_METADATA_FILEPATH, "r") as f:
        audioMetadata = json.loads(f.read())
    for eventName, eventInfo in audioMetadata['Events'].items():
        if "eventID" in eventInfo:
            eventInfo["eventID"] = int(eventInfo["eventID"])
    return audioMetadata