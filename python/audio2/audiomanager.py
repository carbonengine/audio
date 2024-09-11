import audio2

INIT_BANK = "Init.bnk"


class AudioManager(object):
    """Wrapper for Carbon Audio's AudManager class. Provides an easier entry point into CarbonAudio and gives some additional helper methods for loading/unloading SoundBanks.""" 
    def __init__(self, baseSoundbankPath, languageDirectory, applicationName, stereoAudioDeviceName="", spatialAudioDeviceName="", spatialAudioEnabled=True):
        """
        :param baseSoundbankPath: The base SoundBank blue path where all SoundBanks are kept. This path must have been  registered 
                                  as a path using blue.SetSearchPath() and this must follow the blue path convention 
                                  (e.g. "resAudio:/" or "soundbanks:/" or whatever name you registered the path with using blue).
        :type baseSoundbankPath: str
        :param languageDirectory: The language you want to initialize CarbonAudio to use when playing language specific sounds. 
                                  These languages are defined in the Wwise project. Common examples are: "English(US)", "Chinese", "German", etc.
        :type language: str
        :param applicationName: The name of the application using CarbonAudio. This is the name that will show up when using the Wwise profiler.
        :type applicationName: str
        :param stereoAudioDeviceName: An optional parameter that tells Carbon Audio the name of the stereo audio device in your Wwise project.
                                      If not set then it defaults to "System_Stereo". If misconfigured then it may not be possible to manually
                                      turn off spatial (also called "3D") audio in game.
        :type stereoAudioDeviceName: str
        :param spatialAudioDeviceName: An optional parameter that tells Carbon Audio the name of your Wwise project's 3D enabled audio device.
                                       If not set then it defaults to "System". If misconfigured then spatial (also called "3D") audio will not work.
        :type spatialAudioDeviceName: str
        :param spatialAudioEnabled: An optional parameter that signals you want spatial audio enabled when Carbon Audio is initialized. Defaults to True.
                                    Note: even if this is set to True, spatial audio will still not work if the user's current output device is not
                                    configured to leverage a spatial audio endpoint (e.g. Dolby Atmos or Windows Sonic for Headphones).
        :type spatialAudioDeviceName: bool 
        """
        self.defaultSoundBanks = []
        self.manager = audio2.GetOrCreateManager()
        self.staticDataRepository = audio2.GetStaticDataRepository()
        self.banksWaitingToLoad = set()
        self.enabled = False

        self.settings = self._CreateAudioSettings(
            baseSoundbankPath, 
            languageDirectory, 
            applicationName, 
            stereoAudioDeviceName=stereoAudioDeviceName, 
            spatialAudioDeviceName=spatialAudioDeviceName,
            spatialAudioEnabled=spatialAudioEnabled
        )
        self.manager.UpdateSettings(self.settings)

    def AddAndLoadDefaultSoundBank(self, soundBankName):
        """Add a SoundBank as a default SoundBank so it never gets unloaded and then load it."""
        if soundBankName not in self.defaultSoundBanks:
            self.defaultSoundBanks.append(soundBankName)
            self.LoadSoundBank(soundBankName)

    def Disable(self):
        """Disable the audio manager and unload all SoundBanks from memory."""
        self.banksWaitingToLoad = set(self.GetLoadedSoundBanks())
        self.manager.Disable()
        self.enabled = False

    def DisableSoundPrioritization(self):
        self.manager.DisableAudioCulling()

    def DisableSpatialAudio(self):
        """Disable spatial audio (sometimes referred to as 3D audio). This takes effect even if the user has an active spatial endpoint (e.g Dolby Atmos, Sonic for Headphones, etc.).""" 
        return self.manager.DisableSpatialAudio()

    def Enable(self, soundBanksToLoad=[]):
        """Enable the audio manager, load the init bank and any default SoundBanks to memory, enable sound prioritization and create a listener.

        :param soundBanksToLoad: A list of SoundBanks to load when enabling the sound engine. Note: The default soundbanks defined while initializing this 
                                 audio manager will always be loaded when enabling so this is only for those SoundBanks that are not part of the defaults.
        :type soundBanksToLoad: list
        """
        self.manager.Enable(self.defaultSoundBanks + soundBanksToLoad + list(self.banksWaitingToLoad))
        audio2.GetListener()
        self.enabled = True
        self.banksWaitingToLoad = set()

    def EnableSoundPrioritization(self):
        """Enable sound prioritization. Initialize must be called before enabling this."""
        self.manager.EnableAudioCulling()

    def EnableSpatialAudio(self):
        """Enable the use of any active spatial audio endpoints (e.g. Dolby Atmos, Sonic for Headphones, etc.). If the user does not have a spatial audio endpoint active then it will do nothing."""
        return self.manager.EnableSpatialAudio()

    def GetAudioEmitter(self, emitterID):
        return self.manager.GetAudioEmitter(emitterID)

    def GetLoadedSoundBanks(self):
        """Returns the names of all banks loaded into memory."""
        return self.manager.GetLoadedSoundBanks()

    def GetSoundPrioritizationEnabled(self):
        return self.manager.audioCullingEnabled

    def GetSpatialAudioEnabled(self):
        """Return whether spatial audio (sometimes referred to as 3D audio) is enabled."""
        return self.manager.spatialAudioEnabled

    def Initialize(self, eventMetadata, defaultSoundBanks=[]):
        """Initialize the audio manager so that is ready to be enabled.

        :param eventMetadata: Metadata about all Wwise that will ever be used while Carbon Audio is running. Used for sound prioritization 
                              and ideally generated using Waapi (Wwise authoring api).
        :type eventMetadata: dict
        :param defaultSoundBanks: An optional argument that signals specific soundbanks that should always be loaded while the audio manager is enabled.
                                  These SoundBanks will only ever be unloaded when disabling the audio manager.
        :type defaultSoundBanks: list of strings
        """
        self.defaultSoundBanks = defaultSoundBanks
        self.staticDataRepository.Initialize(eventMetadata)

    def LoadSoundBank(self, bankName):
        if not self.enabled:
            self.banksWaitingToLoad.add(bankName)
        else:    
            self.manager.LoadBank(bankName)

    def LoadSoundBanks(self, banksToLoad):
        for bank in banksToLoad:
            self.LoadSoundBank(bank)

    def RegisterAudioDeviceChangeCallback(self, callback):
        """Registers a callback that will be called every time Wwise or the user's audio device changes to determine if the user's audio output supports spatial audio.

        The audio device change callback is called in the following instances:  
            * When initializing carbon audio
			* When enabling/disabling spatial audio manually 
            * When the system's audio output changes (such as changing from headpones to speakers). 

        :param callback: This callback allows the consumer of Carbon Audio to know if the user's system supports spatial audio or not.
                         The callback must accept a boolean argument that signifies whether or not the user's current audio output supports
                         spatial audio. An example of a valid callback would be:
			             ```
                         def audioDeviceChangeCallback(outputSupportsSpatialAudio):
			                    if outputSupportsSpatialAudio:
			                        print('The user's current output device supports spatial audio.')
			                    else:
			                        print('The user's current output device does not support spatial audio.)
        :type callback: function
        """
        self.manager.RegisterAudioDeviceChangeCallback(callback)

    def ReloadSoundBanks(self):
        """Hot reload the currently loaded SoundBanks.

        :return: A list of SoundBanks that failed to load if any, otherwise an empty list.
        """
        banksBeforeReload = self.manager.GetLoadedSoundBanks()
        self.Disable()
        self.Enable(soundBanksToLoad=banksBeforeReload)

    def RemoveAndUnloadDefaultSoundBank(self, soundBankName):
        """Remove a SoundBank as a default SoundBank and unload it."""
        if soundBankName in self.defaultSoundBanks:
            self.defaultSoundBanks.remove(soundBankName)
            self.UnloadSoundBank(soundBankName)

    def SetGlobalRTPC(self, rtpcName, value):
        """Set an RTPC value not tied to a specific emitter.

        :param rtpcName: The RTPCs name in Wwise.
        :type rtpcName: str
        :param value: A value between 0 and 1 to set the RTPC to.
        :type value: float
        """
        return self.manager.SetGlobalRTPC(rtpcName, value)

    def SetState(self, stateGroup, stateName):
        """ Set a global Wwise state.
        :param stateGroup: The state group whose state you want to set.
        :type stateGroup: str
        :param stateName: The name of the state you want to set.
        :type stateName: str
        """
        return self.manager.SetState(stateGroup, stateName)

    def SpatialAudioIsSupported(self):
        """Can be used to determine if Carbon Audio supports spatial audio on this operating system."""
        return self.manager.SpatialAudioIsSupported()

    def StopAllPlayingSounds(self):
        """Stops all playing sounds from every audio emitter."""
        self.manager.StopAll()

    def SwapSoundBanks(self, banksToLoad):
        """Unload all soundbanks that are not default and not in banksToLoad, then load whatever is left in banksToLoad.

        :type banks: list
        :param banks: A list of banks you want to have loaded in addition to the default banks.
        """ 
        if self.enabled:
            loadedBanks = set(self.GetLoadedSoundBanks())
        else:
            loadedBanks = self.banksWaitingToLoad

        excludedBanks = set(self.defaultSoundBanks).union(set(banksToLoad))
        banksToUnload = loadedBanks.difference(excludedBanks) 
        banksToLoad = set(banksToLoad).difference(loadedBanks) 
        self.LoadSoundBanks(banksToLoad)
        self.UnloadSoundBanks(banksToUnload)

    def UnloadSoundBanks(self, soundBanksToUnload):
        """Unload all SoundBanks except for the init SoundBank and default SoundBanks."""
        for soundBank in soundBanksToUnload:
            self.UnloadSoundBank(soundBank)

    def UnloadSoundBank(self, bankName):
        if bankName not in self.defaultSoundBanks and bankName != INIT_BANK:
            if not self.enabled:
                if bankName in self.banksWaitingToLoad:
                    self.banksWaitingToLoad.remove(bankName)

            self.manager.UnloadBank(bankName)

    def _CreateAudioSettings(self, baseSoundbankPath, language, applicationName, stereoAudioDeviceName="", spatialAudioDeviceName="", spatialAudioEnabled=True):
        """Create audio settings to be used by CarbonAudio.

        :param baseSoundbankPath: The base SoundBank blue path where all SoundBanks are kept. This path must have been  registered 
                                  as a path using blue.SetSearchPath() and this must follow the blue path convention 
                                  (e.g. "resAudio:/" or "soundbanks:/" or whatever name you registered the path with using blue).
        :type baseSoundbankPath: str
        :param languageDirectory: The language you want to initialize CarbonAudio to use when playing language specific sounds. 
                                  These languages are defined in the Wwise project. Common examples are: "English(US)", "Chinese", "German", etc.
        :type language: str
        :param applicationName: The name of the application using CarbonAudio. This is the name that will show up when using the Wwise profiler.
        :type applicationName: str
        :param stereoAudioDeviceName: An optional parameter that tells Carbon Audio the name of the stereo audio device in your Wwise project.
                                      If not set then it defaults to "System_Stereo". If misconfigured then it may not be possible to manually
                                      turn off spatial (also called "3D") audio in game.
        :type stereoAudioDeviceName: str
        :param spatialAudioDeviceName: An optional parameter that tells Carbon Audio the name of your Wwise project's 3D enabled audio device.
                                       If not set then it defaults to "System". If misconfigured then spatial (also called "3D") audio will not work.
        :type spatialAudioDeviceName: str
        :param spatialAudioEnabled: An optional parameter that signals you want spatial audio enabled when Carbon Audio is initialized. Defaults to True.
                                    Note: even if this is set to True, spatial audio will still not work if the user's current output device is not
                                    configured to leverage a spatial audio endpoint (e.g. Dolby Atmos or Windows Sonic for Headphones).
        :type spatialAudioDeviceName: bool 
        :return: An audio2.AudSettings instance with the given values.
        """
        settings = audio2.AudSettings()
        settings.applicationName = applicationName
        settings.baseSoundbankPath = baseSoundbankPath
        settings.soundbankLanguage = language 
        settings.spatialAudioEnabled = spatialAudioEnabled

        if stereoAudioDeviceName:
            settings.stereoAudioDeviceName = stereoAudioDeviceName

        if spatialAudioDeviceName:
            settings.spatialAudioDeviceName = spatialAudioDeviceName

        return settings 
