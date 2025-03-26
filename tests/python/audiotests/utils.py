import blue
import json
import os

AUDIO_METADATA_FILEPATH = os.path.abspath(os.path.join(os.path.dirname(__file__), "test", "soundbanks", "SoundPrioritizationMetadata.json"))
def PumpOSWithTimeout(booleanFunc, maxTries=10):
    numTries = 0
    while( numTries < maxTries and booleanFunc() ):
        blue.pyos.synchro.SleepWallclock(100)
        blue.os.Pump()
        numTries += 1

def GetAudioMetadataFromFile():
    """Gets audio metadata from file and returns it as a dict. Also converts eventIDs to long in the process."""
    with open(AUDIO_METADATA_FILEPATH, "r") as f:
        audioMetadata = json.loads(f.read())
    for eventName, eventInfo in audioMetadata['Events'].items():
        if "eventID" in eventInfo:
            try:
                eventInfo["eventID"] = long(eventInfo["eventID"]) # use long() in Python 2
            except NameError:
                eventInfo["eventID"] = int(eventInfo["eventID"]) # Use int() in Python 3
    return audioMetadata