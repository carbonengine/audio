import json

import blue
from const import AUDIO_METADATA_FILEPATH

def PumpOSWithTimeout(booleanFunc, maxTries=10):
    numTries = 0
    while( numTries < maxTries and booleanFunc() ):
        blue.pyos.synchro.SleepWallclock(100)
        blue.os.Pump()
        numTries += 1


def GetEventMetadataFromFile():
    """Get event metadata from file and returns it as a dict. Also converts eventIDs to long in the process."""
    with open(AUDIO_METADATA_FILEPATH, "r") as f:
        audioMetadata = json.loads(f.read())

    # eventID's have to be long or else CarbonAudio doesn't correctly grab it.
    for eventName, eventInfo in audioMetadata.iteritems():
        eventInfo["eventID"] = long(eventInfo["eventID"])

    return audioMetadata
