// Copyright © 2026 CCP ehf.

#include "SoundBankFolder.h"

#include "Platform.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <charconv>
#include <cstdlib>
#include <fstream>
#include <map>
#include <set>

namespace
{
	using Json = nlohmann::json;
	namespace fs = std::filesystem;

	const char* ESSENTIAL_FOLDER = "Essential_Media";
	/// The lab project's speech, which new emitters play when the folder has it.
	const char* SPEECH_EVENT = "Play_AudioLab_Speech";

	std::string GetString( const Json& object, const char* key )
	{
		const auto it = object.find( key );
		return it != object.end() && it->is_string() ? it->get<std::string>() : std::string();
	}

	bool GetBool( const Json& object, const char* key )
	{
		const auto it = object.find( key );
		return it != object.end() && it->is_boolean() && it->get<bool>();
	}

	const Json& GetArray( const Json& object, const char* key )
	{
		static const Json empty = Json::array();
		const auto it = object.find( key );
		return it != object.end() && it->is_array() ? *it : empty;
	}

	std::vector<std::wstring> GetStrings( const Json& object, const char* key )
	{
		std::vector<std::wstring> strings;
		for( const Json& item : GetArray( object, key ) )
		{
			if( item.is_string() )
			{
				strings.push_back( AudioLab::ToWide( item.get<std::string>() ) );
			}
		}
		return strings;
	}

	/// A whole unsigned decimal number, as Wwise writes IDs; false for anything else.
	bool ParseID( const std::string& text, uint32_t& out )
	{
		const char* end = text.data() + text.size();
		const auto result = std::from_chars( text.data(), end, out );
		return !text.empty() && result.ec == std::errc() && result.ptr == end;
	}

	/// A whole decimal number, as Wwise writes distances; false for anything else.
	bool ParseNumber( const std::string& text, double& out )
	{
		char* end = nullptr;
		out = std::strtod( text.c_str(), &end );
		return !text.empty() && end == text.c_str() + text.size();
	}

	bool ReadJson( const fs::path& file, Json& out )
	{
		std::ifstream stream( file, std::ios::binary );
		if( !stream )
		{
			return false;
		}
		std::string text( ( std::istreambuf_iterator<char>( stream ) ), std::istreambuf_iterator<char>() );
		if( text.size() >= 3 && text.compare( 0, 3, "\xEF\xBB\xBF" ) == 0 )
		{
			text.erase( 0, 3 );
		}
		out = Json::parse( text, nullptr, false );
		return !out.is_discarded();
	}

	/// A file is essential when it is in Essential_Media and newer than any copy in the root.
	bool IsEssential( const fs::path& folder, const fs::path& fileName )
	{
		std::error_code error;
		const fs::path essential = folder / ESSENTIAL_FOLDER / fileName;
		if( !fs::exists( essential, error ) )
		{
			return false;
		}
		const fs::path root = folder / fileName;
		if( !fs::exists( root, error ) )
		{
			return true;
		}
		return fs::last_write_time( essential, error ) > fs::last_write_time( root, error );
	}

	uint64_t Hash( uint64_t hash, uint64_t value )
	{
		for( int i = 0; i < 8; ++i )
		{
			hash ^= ( value >> ( i * 8 ) ) & 0xFF;
			hash *= 1099511628211ull;
		}
		return hash;
	}

	uint64_t HashFolder( uint64_t hash, const fs::path& folder )
	{
		std::error_code error;
		std::vector<std::pair<std::string, uint64_t>> entries;
		for( fs::directory_iterator it( folder, error ), end; !error && it != end; it.increment( error ) )
		{
			const fs::path& file = it->path();
			const std::string extension = file.extension().string();
			if( extension != ".bnk" && extension != ".json" && extension != ".wem" )
			{
				continue;
			}
			std::error_code fileError;
			const uint64_t size = static_cast<uint64_t>( it->file_size( fileError ) );
			const uint64_t time = static_cast<uint64_t>( it->last_write_time( fileError ).time_since_epoch().count() );
			entries.emplace_back( AudioLab::ToUtf8Path( file.filename() ), Hash( size, time ) );
		}
		std::sort( entries.begin(), entries.end() );
		for( const auto& entry : entries )
		{
			for( const char character : entry.first )
			{
				hash = Hash( hash, static_cast<unsigned char>( character ) );
			}
			hash = Hash( hash, entry.second );
		}
		return hash;
	}
}

namespace AudioLab
{
	bool LoadSoundBankFolder( const std::filesystem::path& path, SoundBankFolder& out, std::string& error )
	{
		out = SoundBankFolder();
		out.path = path;

		const fs::path infoFile = path / "SoundbanksInfo.json";
		Json root;
		if( !ReadJson( infoFile, root ) )
		{
			error = "Cannot read " + ToUtf8Path( infoFile ) + ". Generate the SoundBanks of tools/audiolab/Wwise/AudioLab, or pass --soundbanks <folder>.";
			return false;
		}
		const auto info = root.find( "SoundBanksInfo" );
		if( info == root.end() || !info->is_object() )
		{
			error = ToUtf8Path( infoFile ) + " has no SoundBanksInfo section.";
			return false;
		}

		struct EventEntry
		{
			CarbonAudioHost::EventInfo info;
			std::set<std::string> mediaIDs;
			bool hasAttenuation = false;
		};
		std::map<std::string, EventEntry> events;
		std::map<std::string, std::vector<std::wstring>> banksOfMedia;

		for( const Json& bank : GetArray( *info, "SoundBanks" ) )
		{
			const std::string fileName = GetString( bank, "Path" );
			if( fileName.empty() )
			{
				continue;
			}
			const std::wstring bankName = ToWide( fileName );
			out.staticData.soundBanks.emplace_back( bankName, IsEssential( path, FromUtf8Path( fileName ) ) );
			if( fileName != "Init.bnk" )
			{
				out.soundBanks.push_back( bankName );
			}

			// Anything that is not as Wwise writes it is left out with a warning, so a hand-edited file cannot stop the lab.
			for( const Json& media : GetArray( bank, "Media" ) )
			{
				const std::string id = GetString( media, "Id" );
				banksOfMedia[id].push_back( bankName );
				if( GetString( media, "Streaming" ) == "true" )
				{
					uint32_t sourceID = 0;
					if( !ParseID( id, sourceID ) )
					{
						out.warnings.push_back( "Streamed media \"" + id + "\" of " + fileName + " has no numeric ID; it is left out." );
						continue;
					}
					const std::wstring key = std::to_wstring( static_cast<int32_t>( sourceID ) );
					out.staticData.sources.emplace_back( key, IsEssential( path, FromUtf8Path( id + ".wem" ) ) );
				}
			}

			for( const Json& event : GetArray( bank, "Events" ) )
			{
				const std::string name = GetString( event, "Name" );
				uint32_t eventID = 0;
				if( !ParseID( GetString( event, "Id" ), eventID ) )
				{
					out.warnings.push_back( "Event \"" + name + "\" of " + fileName + " has no numeric ID; it is left out." );
					continue;
				}
				EventEntry& entry = events[name];
				entry.info.name = ToWide( name );
				entry.info.eventID = eventID;
				const std::string attenuation = GetString( event, "MaxAttenuation" );
				if( !attenuation.empty() )
				{
					if( ParseNumber( attenuation, entry.info.maxAttenuationRadius ) )
					{
						entry.hasAttenuation = true;
					}
					else
					{
						out.warnings.push_back( "Event \"" + name + "\" has a MaxAttenuation of \"" + attenuation + "\"; it counts as 2D." );
					}
				}
				const std::string duration = GetString( event, "DurationType" );
				entry.info.isLoop = !duration.empty() && duration != "OneShot";
				entry.info.soundBanks.push_back( bankName );
				for( const Json& reference : GetArray( event, "MediaRefs" ) )
				{
					entry.mediaIDs.insert( GetString( reference, "Id" ) );
				}
			}

			for( const Json& bus : GetArray( bank, "AuxBusses" ) )
			{
				out.auxBusses.push_back( GetString( bus, "Name" ) );
			}
			const auto plugins = bank.find( "Plugins" );
			if( plugins != bank.end() && plugins->is_object() )
			{
				for( const Json& device : GetArray( *plugins, "AudioDevices" ) )
				{
					out.audioDevices.push_back( GetString( device, "Name" ) );
				}
			}
		}

		for( auto& [name, entry] : events )
		{
			for( const std::string& mediaID : entry.mediaIDs )
			{
				for( const std::wstring& bank : banksOfMedia[mediaID] )
				{
					if( std::find( entry.info.soundBanks.begin(), entry.info.soundBanks.end(), bank ) == entry.info.soundBanks.end() )
					{
						entry.info.soundBanks.push_back( bank );
					}
				}
			}
			entry.info.is2D = !entry.hasAttenuation;
		}

		const fs::path metadataFile = path / "SoundPrioritizationMetadata.json";
		Json metadata;
		if( ReadJson( metadataFile, metadata ) && metadata.is_object() )
		{
			const auto metadataEvents = metadata.find( "Events" );
			if( metadataEvents != metadata.end() && metadataEvents->is_object() )
			{
				for( const auto& item : metadataEvents->items() )
				{
					const auto it = events.find( item.key() );
					if( it == events.end() )
					{
						continue;
					}
					const Json& value = item.value();
					CarbonAudioHost::EventInfo& event = it->second.info;
					event.isLoop = GetBool( value, "isLoop" );
					event.is2D = GetBool( value, "is2D" );
					event.isVital = GetBool( value, "isVital" );
					event.stoppedBy = GetStrings( value, "eventsStoppedBy" );
					std::vector<std::wstring> banks = GetStrings( value, "soundbanks" );
					if( !banks.empty() )
					{
						event.soundBanks = std::move( banks );
					}
				}
			}
		}

		for( auto& [name, entry] : events )
		{
			out.eventNames.push_back( name );
			out.staticData.events.push_back( std::move( entry.info ) );
		}

		const auto hall = std::find( out.auxBusses.begin(), out.auxBusses.end(), "AudioLab_Hall" );
		if( hall != out.auxBusses.end() )
		{
			std::rotate( out.auxBusses.begin(), hall, hall + 1 );
		}
		return true;
	}

	uint64_t GetSoundBankFolderFingerprint( const std::filesystem::path& path )
	{
		std::error_code error;
		if( !fs::exists( path / "SoundbanksInfo.json", error ) )
		{
			return 0;
		}
		uint64_t hash = 14695981039346656037ull;
		hash = HashFolder( hash, path );
		hash = HashFolder( hash, path / ESSENTIAL_FOLDER );
		return hash;
	}

	std::string PickLoopEvent( const SoundBankFolder& folder )
	{
		std::string first;
		for( const CarbonAudioHost::EventInfo& event : folder.staticData.events )
		{
			if( event.isLoop && !event.is2D )
			{
				const std::string name = ToUtf8( event.name );
				if( name == SPEECH_EVENT )
				{
					return name;
				}
				first = first.empty() ? name : first;
			}
		}
		return first;
	}

	float GetEventRadius( const SoundBankFolder& folder, const std::string& eventName )
	{
		const std::wstring name = ToWide( eventName );
		for( const CarbonAudioHost::EventInfo& event : folder.staticData.events )
		{
			if( event.name == name )
			{
				return static_cast<float>( event.maxAttenuationRadius );
			}
		}
		return 0.0f;
	}
}
