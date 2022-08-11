# audio2
audio2 is the middleware through which EVE Online communicates with Wwise. 
audio2 is accesible in two ways: 
1. Through blue and usable by python.
2. Through interfaces defined in Trinity.

## Testing
audio2 is tested through its Python interface located at `<BRANCH_ROOT>/packages/audio2`. 
All tests can be found in the `<BRANCH_ROOT>/packages/audio2/tests` directory.

### Running audio2 tests

The tests for audio2 are run using the python test suite located at `<BRANCH_ROOT>/testsuites/eve_testsuite.py`.
To run the tests open powershell and `cd` to the root of the EVE branch. Once there you will first need to update
all the EVE binaries by running 
```
python .\updateBinaries.py -d client
```
Then run the tests using the following command:
```
.\eve\client\pythonInterpreter.bat .\testsuites\eve_testsuite.py --filters=audio2
```

Keep in mind that every time you make a change to audio2 and rebuild then you must run `updateBinaries.py` again.

## Useful paths to know about for audio development in EVE.
The following places in EVE that relate to audio development:

| Path   | What can be found there |
| ---    | ---                   |
| `<branch-root>/packages/audiotools`| The Wwise project |
| `<branch-root>/packages/eveaudio`  | Python logic for audio to be used in game. In particular this is where the music system logic sits.
| `<branch-root>/carbon/tools/audiotools` | Audio related scripts for TeamCity or other uses, Wwise authoring libraries for in use Wwise versions, any other tools needed by sound designers or TeamCity. |
| `//depot/content/EVE/Audio/EVE-Audio/Originals/` | Where all the source .wav files sit in Perforce.|