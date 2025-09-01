#include "stdafx.h"
#include "AudStaticDataRepository.h"

namespace
{

	PyObject* GetPyObjectFromDictionary(PyObject* dict, const char* key, const std::wstring& name)
	{
		PyObject* item = PyDict_GetItemString(dict, key);
		if (item != nullptr)
		{
			return item;
		}
		else
		{
			CCP_LOGERR("Could not find key %s for %S while generating sound ID lookup table.", key, name.c_str());
			return nullptr;
		}
	}

	// Helper function to generate a vector of wstrings from a Python list
	std::vector<std::wstring> GenerateVectorFromPythonList(PyObject* pyList)
	{
		std::vector<std::wstring> newVector = std::vector<std::wstring>();
		if (pyList != nullptr)
		{
			if (PyList_CheckExact(pyList))
			{
				const unsigned int listLength = (unsigned int)PyList_GET_SIZE(pyList);
				for (unsigned int i = 0; i < listLength; i++)
				{
#if PY_MAJOR_VERSION == 2
					std::string valueC = PyString_AsString(PyList_GetItem(pyList, i));
#else
					std::string valueC = PyUnicode_AsUTF8(PyList_GetItem(pyList, i));
#endif
					std::wstring value = static_cast<const wchar_t*>(CA2W(valueC.c_str()));
					newVector.push_back(value);
				}
			}
		}

		return newVector;
	}

	// Get a boolean value from a Python dictionary using the specified key and name.
	bool GetBoolFromDict(PyObject* dict, const char* key, const std::wstring& name)
	{
		PyObject* pyObj = GetPyObjectFromDictionary(dict, key, name);
		if (pyObj != nullptr && PyObject_IsTrue(pyObj))
		{
			return true;
		}
		return false;
	}

	unsigned int GetUIntFromDict(PyObject* dict, const char* key, const std::wstring& name)
	{
		PyObject* pyObj = GetPyObjectFromDictionary(dict, key, name);
		if (pyObj != nullptr && PyLong_Check(pyObj))
		{
			return PyLong_AsUnsignedLong(pyObj);
		}
		return 0;
	}

	float GetFloatFromDict(PyObject* dict, const char* key, const std::wstring& name)
	{
		PyObject* pyObj = GetPyObjectFromDictionary(dict, key, name);
		if (pyObj != nullptr && PyFloat_Check(pyObj))
		{
			return PyFloat_AsDouble(pyObj);
		}
		return 0.0f;
	}

	std::vector<std::wstring> GetVectorFromDict(PyObject* dict, const char* key, const std::wstring& name)
	{
		PyObject* pyObj = GetPyObjectFromDictionary(dict, key, name);
		return GenerateVectorFromPythonList(pyObj);
	}
}

AudStaticDataRepository::AudStaticDataRepository(IRoot* lockobj) :
	m_events({}),
	m_soundBanks({}),
	m_sources({}),
	m_initialized(false),
	m_staticDataMutex("AudStaticDataRepository", "m_staticDataMutex")
{
}

bool AudStaticDataRepository::IsInitialized() const
{
	return m_initialized;
}

unsigned int AudStaticDataRepository::GetEventID(const std::wstring& eventName) const
{
	return GetAttribute<EventData, unsigned int>(eventName, m_events, m_staticDataMutex, &EventData::eventID, AK_INVALID_UNIQUE_ID);
}

bool AudStaticDataRepository::EventIsLoop(const std::wstring& eventName) const
{
	return GetAttribute<EventData, bool>(eventName, m_events, m_staticDataMutex, &EventData::isLoop, false);
}

float AudStaticDataRepository::GetEventRadiusSq(const std::wstring& eventName) const
{
	CcpAutoMutex mutex(m_staticDataMutex);
	const EventData* eventData = GetData(eventName, m_events, m_staticDataMutex);
	if (eventData != nullptr)
	{
		float radius = eventData->maxAttenuationRadius;
		return radius * radius;
	}
	return 0.0f;
}

bool AudStaticDataRepository::EventIs2D(const std::wstring& eventName) const
{
	return GetAttribute<EventData, bool>(eventName, m_events, m_staticDataMutex, &EventData::is2D, false);
}

bool AudStaticDataRepository::EventIsVital(const std::wstring& eventName) const
{
	return GetAttribute<EventData, bool>(eventName, m_events, m_staticDataMutex, &EventData::isVital, false);
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
bool AudStaticDataRepository::EventIsStopped(const std::wstring& eventPotentiallyStopped, const std::wstring& eventPotentiallyStopping) const
{
	CcpAutoMutex mutex(m_staticDataMutex);
	const EventData* eventData = GetData(eventPotentiallyStopped, m_events, m_staticDataMutex);
	if (eventData != nullptr)
	{
		auto result = std::find(eventData->eventsStoppedBy.begin(), eventData->eventsStoppedBy.end(), eventPotentiallyStopping);
		if (result != eventData->eventsStoppedBy.end())
		{
			return true;
		}
	}
	return false;
}

bool AudStaticDataRepository::SourceIsEssential(AkInt32 sourceID) const
{
	return GetAttribute(std::to_wstring(sourceID), m_sources, m_staticDataMutex, &SourceData::isEssential, false);
}

bool AudStaticDataRepository::SoundBankIsEssential(const std::wstring& soundBankName) const
{
	return GetAttribute(soundBankName, m_soundBanks, m_staticDataMutex, &SoundBankData::isEssentialSoundBank, false);
}


const std::vector<std::wstring>& AudStaticDataRepository::SoundBanksRequiredForEvent(const std::wstring& eventName) const
{
    static const std::vector<std::wstring> emptyVector;
    
    const EventData* eventData = GetData(eventName, m_events, m_staticDataMutex);
    if (eventData != nullptr)
    {
        return eventData->soundbanks;
    }
    return emptyVector;
}

//-----------------------------------------------------------------------------
// Description:
//   Generate a look up table based off sound ID static data provided by python. Sets the m_initialized flag to true if successful.
//   This function must be called and is necessary for audio2 to function correctly if audio culling is enabled.
//-----------------------------------------------------------------------------
void AudStaticDataRepository::Initialize(PyObject* audioMetadata)
{
	if (!PyDict_Check(audioMetadata))
	{
		CCP_LOGERR("AudStaticDataRepository::Initialize expects audioMetadata as a dictionary but didn't receive a dictionary");
		return;
	}

	PyObject* key, * value = NULL;
	Py_ssize_t pos = 0;

	const char* eventDictNameC = "Events";
	std::wstring eventDictName = static_cast<const wchar_t*>(CA2W(eventDictNameC));
	PyObject* eventsDict = GetPyObjectFromDictionary(audioMetadata, eventDictNameC, eventDictName);

	CcpAutoMutex mutex(m_staticDataMutex);
	if (eventsDict != nullptr && PyDict_Check(eventsDict))
	{
		while (PyDict_Next(eventsDict, &pos, &key, &value))
		{
#if PY_MAJOR_VERSION == 2
			std::string eventNameC = PyString_AsString(key);
#else
			std::string eventNameC = PyUnicode_AsUTF8(key);
#endif
			std::wstring eventName = static_cast<const wchar_t*>(CA2W(eventNameC.c_str()));

			bool isLoop = GetBoolFromDict(value, "isLoop", eventName);
			bool is2D = GetBoolFromDict(value, "is2D", eventName);
			bool isVital = GetBoolFromDict(value, "isVital", eventName);

			unsigned int eventID = GetUIntFromDict(value, "eventID", eventName);
			float maxAttenuationRadius = GetFloatFromDict(value, "maxRadiusAttenuation", eventName);

			std::vector<std::wstring> eventsStoppedBy = GetVectorFromDict(value, "eventsStoppedBy", eventName);
			std::vector<std::wstring> soundbanks = GetVectorFromDict(value, "soundbanks", eventName);

			EventData eventData = {
				eventName,
				eventID,
				maxAttenuationRadius,
				isLoop,
				is2D,
				isVital,
				eventsStoppedBy,
				soundbanks
			};
			m_events[eventName] = eventData;
		}
	}
	else
	{
		CCP_LOGERR("Initializing audio static data for Events failed, make sure the data provided to AudStaticDataRepository is in the correct format.");
	}

	key, value = nullptr;
	pos = 0;

	const char* soundbankDictNameC = "SoundBanks";
	std::wstring soundbankDictName = static_cast<const wchar_t*>(CA2W(soundbankDictNameC));
	PyObject* soundbanksDict = GetPyObjectFromDictionary(audioMetadata, soundbankDictNameC, soundbankDictName);

	if (soundbanksDict != nullptr && PyDict_Check(soundbanksDict))
	{
		CcpAutoMutex src_mutex(m_staticDataMutex);
		while (PyDict_Next(soundbanksDict, &pos, &key, &value))
		{
#if PY_MAJOR_VERSION == 2
			std::string soundBankNameC = PyString_AsString(key);
#else
			std::string soundBankNameC = PyUnicode_AsUTF8(key);
#endif
			std::wstring soundBankName = static_cast<const wchar_t*>(CA2W(soundBankNameC.c_str()));

			bool IsEssentialSoundbank = GetBoolFromDict(value, "EssentialSoundBank", soundBankName);

			SoundBankData soundBankData = {
				IsEssentialSoundbank
			};
			m_soundBanks[soundBankName] = soundBankData;
		}
	}
	else
	{
		CCP_LOGERR("Initializing audio static data for Soundbanks failed, make sure the data provided to AudStaticDataRepository is in the correct format.");
	}

	key, value = nullptr;
	pos = 0;

	const char* sourceDictNameC = "WemFileIDs";
	std::wstring sourceDictName = static_cast<const wchar_t*>(CA2W(sourceDictNameC));
	PyObject* sourceDict = GetPyObjectFromDictionary(audioMetadata, sourceDictNameC, sourceDictName);

	if (sourceDict != nullptr && PyDict_Check(sourceDict))
	{
		CcpAutoMutex soundBankMutex(m_staticDataMutex);
		while (PyDict_Next(sourceDict, &pos, &key, &value))
		{
#if PY_MAJOR_VERSION == 2
			std::string sourceNameC = PyString_AsString(key);
#else
			std::string sourceNameC = PyUnicode_AsUTF8(key);
#endif
			std::wstring sourceName = static_cast<const wchar_t*>(CA2W(sourceNameC.c_str()));

			bool IsEssential = GetBoolFromDict(value, "IsEssential", sourceName);

			SourceData sourceData = {
				IsEssential,
			};
			m_sources[sourceName] = sourceData;
		}
	}
	else
	{
		CCP_LOGERR("Initializing audio static data for Audio Sources failed, make sure the data provided to AudStaticDataRepository is in the correct format.");
	}
	m_initialized = true;
}