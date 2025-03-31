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
BLUE_CLASS( AudStaticDataRepository ) : public IRoot
{
public:
	AudStaticDataRepository( IRoot* lockobj = NULL );

    EXPOSE_TO_BLUE();

    // Initialize the repository with the wwise event data from EVE. Returns true if the passed in data was as expected.
    void Initialize( PyObject* wwiseEvents );
    // Whether or not this was successfully initialized with static data.
    bool IsInitialized() const;
    // Get an event ID given an events name
    unsigned int GetEventID( const std::wstring& eventName ) const;
    // Get the squared radius of of an events calculated attenuation.
    float GetEventRadiusSq( const std::wstring& eventName ) const;
    // Whether the given event is a loop or a not.
    bool EventIsLoop( const std::wstring& eventName ) const;
    // Whether the given event is a 2D sound or not.
    bool EventIs2D( const std::wstring& eventName ) const;
    // Whether the given event is a 2D sound or not.
    bool EventIsVital( const std::wstring& eventName ) const;
    // Whether an event stops another event.
	bool EventIsStopped( const std::wstring& eventPotentiallyStopped, const std::wstring& eventPotentiallyStopping ) const;
    // Return a list of the SoundBanks the given event needs to be able to be played
    std::vector<std::wstring> SoundBanksRequiredForEvent( const std::wstring& eventName ) const;
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
    bool m_initialized;
    // Helper function to get sound data for a given event name.
    const EventData* GetEventData( const std::wstring& eventName ) const;
    // Helper function to get a key from a python dictionary. Logs and error if the key lookup fails.
    PyObject* GetPyObjectFromDictionary( PyObject* dict, const char* key, const std::wstring* eventName ) const;
    // Helper function to generate a vector of wstrings from a Python list
    std::vector<std::wstring> GenerateVectorFromPythonList( PyObject* pyList );

    // Generated map from wwiseEvents static data given to AudStaticDataRepository when it is initalized.
    std::unordered_map<std::wstring, EventData> m_events;

    // A mutex to be used for manipulating the m_events attribute
	CcpMutex mutable m_eventsMutex;
};

TYPEDEF_BLUECLASS( AudStaticDataRepository );

extern AudStaticDataRepository* g_staticDataRepository;