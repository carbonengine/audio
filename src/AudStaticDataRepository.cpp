#include "stdafx.h"
#include "AudStaticDataRepository.h"

AudStaticDataRepository::AudStaticDataRepository( IRoot* lockobj ) : 
    m_events( {} ),
    m_initialized( false )
{}

bool AudStaticDataRepository::IsInitialized() const
{
    return m_initialized;
}

unsigned int AudStaticDataRepository::GetEventID( const std::wstring& eventName ) const
{
    const EventData* eventData = GetEventData( eventName );
    if ( eventData != nullptr )
    {
        return eventData->eventID; 
    }
    return AK_INVALID_UNIQUE_ID;
}

bool AudStaticDataRepository::EventIsLoop( const std::wstring& eventName ) const
{
    const EventData* eventData = GetEventData( eventName );
    if ( eventData != nullptr )
    {
        return eventData->isLoop;
    }
    return false;
}

float AudStaticDataRepository::GetEventRadiusSq( const std::wstring& eventName ) const
{
    const EventData* eventData = GetEventData( eventName );
    if ( eventData != nullptr )
    {
        float radius = eventData->maxAttenuationRadius; 
        return radius * radius;
    }
    return 0.0f;
}

bool AudStaticDataRepository::EventIs2D( const std::wstring& eventName ) const
{
    const EventData* eventData = GetEventData( eventName );
    if ( eventData != nullptr )
    {
        return eventData->is2D;
    }
    return false;
}

bool AudStaticDataRepository::EventIsVital( const std::wstring& eventName ) const
{
    const EventData* eventData = GetEventData( eventName );
    if ( eventData != nullptr )
    {
        return eventData->isVital;
    }
    return false;
}

//-----------------------------------------------------
// Description:
//   Determine if a given event is stopped by another event. The data used to determine this is found by looking at the 
//   Wwise project during soundbank generation.
// Arguments:
//   eventPotentiallyStopped - The event you want to know is stopped or not by another event. 
//   eventPotentiallyStopping - The event that you want to know if it stops the first event given as the first argument.
// Return:
//   True if the given eventPotentiallyStopped is actually stopped by eventPotentiallyStopping. False otherwise.
//-----------------------------------------------------
bool AudStaticDataRepository::EventIsStopped( const std::wstring& eventPotentiallyStopped, const std::wstring& eventPotentiallyStopping ) const
{
    const EventData* eventData = GetEventData( eventPotentiallyStopped );
    if ( eventData != nullptr )
    {
        auto result = std::find(eventData->eventsStoppedBy.begin(), eventData->eventsStoppedBy.end(), eventPotentiallyStopping );
		if ( result != eventData->eventsStoppedBy.end() )
		{
			return true;	
        }
    }
    return false;
}

//-----------------------------------------------------------------------------
// Description:
//   Generate a look up table based off sound ID static data provided by python. Sets the m_initialized flag to true if successful.
//   This function must be called and is necessary for audio2 to function correctly if audio culling is enabled.
//-----------------------------------------------------------------------------
void AudStaticDataRepository::Initialize( PyObject* wwiseEvents )
{
    if ( !PyDict_Check( wwiseEvents ) )
    {
        CCP_LOGERR( "AudStaticDataRepository::Initialize expects wwiseEvents as a dictionary but didn't receive a dictionary" );
        return;
    }

    PyObject *key, *value = NULL;
    Py_ssize_t pos = 0;

    while ( PyDict_Next( wwiseEvents, &pos, &key, &value ) )
    {
        std::string eventNameC = PyString_AsString( key );
        std::wstring eventName = static_cast<const wchar_t*>( CA2W( eventNameC.c_str() ) );

        bool isLoop = false;
        PyObject* isLoopPyObj = GetPyObjectFromDictionary( value, "isLoop", &eventName );
        if ( isLoopPyObj != nullptr )
        {
            if ( PyObject_IsTrue( isLoopPyObj ) )
            {
                isLoop = true;
            }
        }

        bool is2D = false;
        PyObject* is2DPyObj = GetPyObjectFromDictionary( value, "is2D", &eventName );
        if ( is2DPyObj != nullptr )
        {
            if ( PyObject_IsTrue( is2DPyObj ) )
            {
                is2D = true;
            }
        }

        bool isVital = false;
        PyObject* isVitalPyObj = GetPyObjectFromDictionary( value, "isVital", &eventName );
        if ( isVitalPyObj != nullptr )
        {
            if ( PyObject_IsTrue( isVitalPyObj ) )
            {
                isVital = true;
            }
        }

        unsigned int eventID = 0;
        PyObject* eventIDPyObj = GetPyObjectFromDictionary( value, "eventID", &eventName );
        if ( eventIDPyObj != nullptr )
        {
            if ( PyLong_Check( eventIDPyObj ) )
            {
                eventID = PyLong_AsUnsignedLong( eventIDPyObj );
            }
        }

        float maxAttenuationRadius = 0.0f;
        PyObject* maxAttenuationRadiusPyObj = GetPyObjectFromDictionary( value, "maxRadiusAttenuation", &eventName );
        if ( maxAttenuationRadiusPyObj != nullptr )
        {
            if ( PyFloat_Check( maxAttenuationRadiusPyObj ) )
            {
                maxAttenuationRadius = PyFloat_AsDouble( maxAttenuationRadiusPyObj );
            }
        }

        std::vector<std::wstring> eventsStoppedBy = std::vector<std::wstring>();
        PyObject* eventsStoppedByObj = GetPyObjectFromDictionary( value, "eventsStoppedBy", &eventName );
        if ( eventsStoppedByObj != nullptr )
        {
            if ( PyList_CheckExact( eventsStoppedByObj ) )
            {
				const unsigned int listLength = (unsigned int)PyList_GET_SIZE( eventsStoppedByObj );
		        for( unsigned int i=0; i<listLength; i++ )
				{
					std::string eventNameC = PyString_AsString( PyList_GetItem( eventsStoppedByObj, i ) );
					std::wstring eventName = static_cast<const wchar_t*>( CA2W( eventNameC.c_str() ) );
					eventsStoppedBy.push_back( eventName );
                }
            }
        }

        EventData eventData = {
            eventName,
            eventID,
            maxAttenuationRadius,
            isLoop,
            is2D,
            isVital,
            eventsStoppedBy,
        };
        m_events[eventName] = eventData;
    }

    m_initialized = true;
}

PyObject* AudStaticDataRepository::GetPyObjectFromDictionary( PyObject* dict, const char* key, const std::wstring* eventName ) const
{
    PyObject* item = PyDict_GetItemString( dict, key );
    if ( item != nullptr )
    {
        return item;
    }
    else
    {
        CCP_LOGERR( "Could not find key %s for event %S while generating sound ID lookup table.", key, eventName->c_str() );
        return nullptr;
    }
}

const AudStaticDataRepository::EventData* AudStaticDataRepository::GetEventData( const std::wstring& eventName ) const
{
    auto it = m_events.find( eventName );
    if ( it != m_events.end() )
    {
        return &it->second;
    }
    return nullptr;
}