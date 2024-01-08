import json
from const import AUDIO_METADATA_FILEPATH

def run_in_tasklet(func):
    def wrapped(*args, **kwargs):
        import stackless
        stackless.tasklet(func)(*args, **kwargs)
        assert(stackless.runcount == 2)
        stackless.run()
        if (stackless.runcount != 1):
            raise RuntimeError("Leaking tasklets")
    return wrapped


def GetEventMetadataFromFile():
    """Get event metadata from file and returns it as a dict. Also converts eventIDs to long in the process."""
    with open(AUDIO_METADATA_FILEPATH, "r") as f:
        audioMetadata = json.loads(f.read())

    # eventID's have to be long or else CarbonAudio doesn't correctly grab it.
    for eventName, eventInfo in audioMetadata.iteritems():
        eventInfo["eventID"] = long(eventInfo["eventID"])

    return audioMetadata
