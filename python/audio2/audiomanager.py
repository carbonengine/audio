import audio2

INIT_BANK = "Init.bnk"


class AudioManager(object):
    """Wrapper for Carbon Audio's AudManager class. Provides an easier entry point into CarbonAudio and gives some additional helper methods for loading/unloading SoundBanks.""" 
    def __init__(self, baseSoundbankPath, languageDirectory, applicationName):
        self.defaultSoundBanks = []
        self.manager = audio2.GetOrCreateManager()
        self.settings = self._CreateAudioSettings(baseSoundbankPath, languageDirectory, applicationName)
        self.staticDataRepository = audio2.GetStaticDataRepository()
        self.banksWaitingToLoad = set()
        self.enabled = False

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

    def GetAudioEmitter(self, emitterID):
        return self.manager.GetAudioEmitter(emitterID)

    def GetLoadedSoundBanks(self):
        """Returns the names of all banks loaded into memory."""
        return self.manager.GetLoadedSoundBanks()

    def GetSoundPrioritizationEnabled(self):
        return self.manager.audioCullingEnabled

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

    def _CreateAudioSettings(self, baseSoundbankPath, language, applicationName):
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
        :return: An audio2.AudSettings instance with the given values.
        """
        settings = audio2.AudSettings()
        settings.applicationName = applicationName
        settings.baseSoundbankPath = baseSoundbankPath
        settings.soundbankLanguage = language 

        return settings 