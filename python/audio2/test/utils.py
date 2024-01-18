import blue

def PumpOSWithTimeout(booleanFunc, maxTries=10):
    numTries = 0
    while( numTries < maxTries and booleanFunc() ):
        blue.pyos.synchro.SleepWallclock(100)
        blue.os.Pump()
        numTries += 1
