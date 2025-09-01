////////////////////////////////////////////////////////////////////////////////
//
// Creator: Eric Nielsen
// Created: July 2020
// Copyright (c) 2020, CCP Games
//

#pragma once

#include "Audio2.h"

// ------------------------------------------------------------------------
// Description:
//   A repository of audio related static data that contains necessary metadata from the Wwise project
//   for audio2 to function. Supplies both getters and setters for the static data.
// ------------------------------------------------------------------------
BLUE_CLASS(AudStaticDataRepository) : public IRoot
{
public:
    AudStaticDataRepository(IRoot * lockobj = NULL);

    EXPOSE_TO_BLUE();

    // Initialize the repository with the wwise event data from EVE. Returns true if the passed in data was as expected.
    void Initialize(PyObject* audioMetadata);
    // Whether or not this was successfully initialized with static data.
    bool IsInitialized() const;
    // Get an event ID given an events name
    unsigned int GetEventID(const std::wstring& eventName) const;
    // Get the squared radius of of an events calculated attenuation.
    float GetEventRadiusSq(const std::wstring& eventName) const;
    // Whether the given event is a loop or a not.
    bool EventIsLoop(const std::wstring& eventName) const;
    // Whether the given event is a 2D sound or not.
    bool EventIs2D(const std::wstring& eventName) const;
    // Whether the given event is a 2D sound or not.
    bool EventIsVital(const std::wstring& eventName) const;
    // Whether an event stops another event.
    bool EventIsStopped(const std::wstring& eventPotentiallyStopped, const std::wstring& eventPotentiallyStopping) const;
    // Returns true if the specified audio source is essential, false otherwise.
    bool SourceIsEssential(AkInt32 sourceID) const;
    // Returns true if the specified soundbank is essential, false otherwise.
    bool SoundBankIsEssential(const std::wstring& soundBankName) const;
    // Return a list of the SoundBanks the given event needs to be able to be played
    const std::vector<std::wstring>& SoundBanksRequiredForEvent(const std::wstring& eventName) const;
protected:
    struct EventData
    {
        std::wstring eventName;
        unsigned int eventID;
        double maxAttenuationRadius;
        bool isLoop;
        bool is2D;
        bool isVital;
        std::vector<std::wstring> eventsStoppedBy;
        std::vector<std::wstring> soundbanks;
    };

    struct SoundBankData
    {
        bool isEssentialSoundBank;
    };

    struct SourceData
    {
        bool isEssential;
    };

    bool m_initialized;

    // Retrieves the data associated with the given name from the data map.
    template <typename DataType>
    const DataType* GetData(const std::wstring& name, const std::unordered_map<std::wstring, DataType>& dataMap, CcpMutex& mutex) const
    {
        CcpAutoMutex autoMutex(mutex);
        auto it = dataMap.find(name);
        if (it != dataMap.end())

        {
            return &((*it).second);
        }
        return nullptr;
    }

    // Retrieves the value of a specific attribute from a data map.
    template <typename DataType, typename AttributeType>
    AttributeType GetAttribute(const std::wstring& name, const std::unordered_map<std::wstring, DataType>& dataMap, CcpMutex& mutex, AttributeType DataType::* attribute, AttributeType defaultValue) const
    {
        const DataType* data = GetData(name, dataMap, mutex);
        if (data != nullptr)
        {
            return data->*attribute;
        }
        return defaultValue;
    }

    std::vector<std::wstring> GenerateVectorFromPythonList(PyObject* pyList);

    // Generated map from wwiseEvents static data given to AudStaticDataRepository when it is initalized.
    std::unordered_map<std::wstring, EventData> m_events;
    std::unordered_map<std::wstring, SoundBankData> m_soundBanks;
    std::unordered_map<std::wstring, SourceData> m_sources;

    // A mutex to be used for manipulating audio metadata maps.
    CcpMutex mutable m_staticDataMutex;
};

TYPEDEF_BLUECLASS(AudStaticDataRepository);

extern AudStaticDataRepository* g_staticDataRepository;