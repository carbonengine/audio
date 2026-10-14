// Copyright © 2026 CCP ehf.

#include "WwiseLink.h"

#include "Platform.h"

#include <AK/WwiseAuthoringAPI/AkAutobahn/Client.h>
#include <AK/WwiseAuthoringAPI/waapi.h>

#include <nlohmann/json.hpp>

#include <algorithm>

namespace
{
	using Json = nlohmann::json;
	namespace waapi = ak::wwise;

	constexpr int CONNECT_TIMEOUT_MS = 2000;
	constexpr int CALL_TIMEOUT_MS = 5000;
	constexpr int ATTACH_TIMEOUT_MS = 10000;
	constexpr std::chrono::seconds PROFILER_POLL_INTERVAL( 2 );
	constexpr std::chrono::seconds MAX_RETRY_INTERVAL( 10 );
	constexpr int LOG_INFO = 0;
	constexpr int LOG_WARNING = 2;
	constexpr int LOG_ERROR = 3;
	/// What Call leaves in result when Wwise did not answer within the timeout.
	const char* const NO_REPLY = "no reply";

	Json Parse( const std::string& text )
	{
		Json json = Json::parse( text, nullptr, false );
		return json.is_discarded() ? Json::object() : json;
	}

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

	bool SameFile( const std::string& a, const std::filesystem::path& b )
	{
		std::error_code error;
		return !a.empty() && !b.empty() && std::filesystem::equivalent( AudioLab::FromUtf8Path( a ), b, error );
	}
}

namespace AudioLab
{
	WwiseLink::WwiseLink() = default;

	WwiseLink::~WwiseLink()
	{
		Shutdown();
	}

	void WwiseLink::Start( const Options& options )
	{
		m_options = options;
		m_enabled = options.enabled;
		m_autoAttach = options.autoAttachProfiler;
		m_thread = std::thread( [this] { Run(); } );
	}

	void WwiseLink::Shutdown()
	{
		{
			std::lock_guard<std::mutex> lock( m_mutex );
			m_stop = true;
		}
		m_wake.notify_all();
		if( m_thread.joinable() )
		{
			m_thread.join();
		}
	}

	void WwiseLink::SetEnabled( bool enabled )
	{
		{
			std::lock_guard<std::mutex> lock( m_mutex );
			m_enabled = enabled;
		}
		m_wake.notify_all();
	}

	bool WwiseLink::IsEnabled() const
	{
		std::lock_guard<std::mutex> lock( m_mutex );
		return m_enabled;
	}

	void WwiseLink::SetAutoAttachProfiler( bool enabled )
	{
		std::lock_guard<std::mutex> lock( m_mutex );
		m_autoAttach = enabled;
		m_userDetached = false;
	}

	bool WwiseLink::GetAutoAttachProfiler() const
	{
		std::lock_guard<std::mutex> lock( m_mutex );
		return m_autoAttach;
	}

	void WwiseLink::AttachProfiler()
	{
		Queue( [this] {
			{
				std::lock_guard<std::mutex> lock( m_mutex );
				m_userDetached = false;
			}
			std::string result;
			const Json args = { { "host", m_options.host }, { "appName", m_options.appName } };
			if( !Call( waapi::core::remote::connect, args.dump(), result, ATTACH_TIMEOUT_MS ) )
			{
				SetError( "The Wwise Profiler could not connect to this process: " + result );
			}
			m_nextProfilerPoll = std::chrono::steady_clock::now();
		} );
	}

	void WwiseLink::DetachProfiler()
	{
		Queue( [this] {
			{
				std::lock_guard<std::mutex> lock( m_mutex );
				m_userDetached = true;
			}
			std::string result;
			Call( waapi::core::remote::disconnect, "{}", result, CALL_TIMEOUT_MS );
			m_nextProfilerPoll = std::chrono::steady_clock::now();
		} );
	}

	void WwiseLink::StartCapture()
	{
		Queue( [this] {
			std::string result;
			const bool started = Call( waapi::core::profiler::startCapture, "{}", result, CALL_TIMEOUT_MS );
			std::lock_guard<std::mutex> lock( m_mutex );
			m_status.capturing = started || m_status.capturing;
		} );
	}

	void WwiseLink::StopCapture()
	{
		Queue( [this] {
			std::string result;
			Call( waapi::core::profiler::stopCapture, "{}", result, CALL_TIMEOUT_MS );
			std::lock_guard<std::mutex> lock( m_mutex );
			m_status.capturing = false;
		} );
	}

	void WwiseLink::GenerateSoundBanks()
	{
		Queue( [this] {
			std::string result;
			const Json args = { { "command", "GenerateAllSoundbanksAllPlatforms" } };
			// Wwise may only answer once the generation is done, after the timeout; generationDone says when it is.
			if( !Call( waapi::ui::commands::execute, args.dump(), result, CALL_TIMEOUT_MS ) && result != NO_REPLY )
			{
				SetError( "Wwise did not start generating SoundBanks: " + result );
			}
		} );
	}

	WwiseStatus WwiseLink::GetStatus() const
	{
		std::lock_guard<std::mutex> lock( m_mutex );
		return m_status;
	}

	std::vector<WwiseNotice> WwiseLink::Drain()
	{
		std::lock_guard<std::mutex> lock( m_mutex );
		std::vector<WwiseNotice> notices;
		notices.swap( m_notices );
		return notices;
	}

	void WwiseLink::Queue( Job job )
	{
		{
			std::lock_guard<std::mutex> lock( m_mutex );
			if( m_status.connection != WwiseStatus::Connection::Connected )
			{
				m_status.lastError = "Wwise Authoring is not connected.";
				return;
			}
			m_jobs.push_back( std::move( job ) );
		}
		m_wake.notify_all();
	}

	void WwiseLink::Notify( WwiseNotice notice )
	{
		std::lock_guard<std::mutex> lock( m_mutex );
		m_notices.push_back( std::move( notice ) );
	}

	void WwiseLink::SetError( const std::string& error )
	{
		std::lock_guard<std::mutex> lock( m_mutex );
		m_status.lastError = error;
	}

	bool WwiseLink::Call( const char* uri, const std::string& args, std::string& result, int timeoutMs )
	{
		result.clear();
		if( !m_client || !m_client->Call( uri, args.c_str(), "{}", result, timeoutMs ) )
		{
			if( result.empty() )
			{
				result = NO_REPLY;
			}
			return false;
		}
		return true;
	}

	void WwiseLink::Run()
	{
		std::chrono::seconds retryInterval( 1 );
		for( ;; )
		{
			bool enabled = false;
			{
				std::lock_guard<std::mutex> lock( m_mutex );
				if( m_stop )
				{
					break;
				}
				enabled = m_enabled;
			}

			if( !enabled )
			{
				Disconnect();
				std::unique_lock<std::mutex> lock( m_mutex );
				m_status.connection = WwiseStatus::Connection::Off;
				m_wake.wait_for( lock, std::chrono::milliseconds( 500 ) );
				continue;
			}

			if( !m_client || m_lost )
			{
				if( m_client )
				{
					Disconnect();
					Notify( { WwiseNotice::Kind::Disconnected, LOG_WARNING, "Wwise", "Wwise Authoring disconnected." } );
				}
				if( !Connect() )
				{
					std::unique_lock<std::mutex> lock( m_mutex );
					m_wake.wait_for( lock, retryInterval, [this] { return m_stop || !m_enabled; } );
					retryInterval = std::min( retryInterval * 2, MAX_RETRY_INTERVAL );
					continue;
				}
				retryInterval = std::chrono::seconds( 1 );
			}

			std::deque<Job> jobs;
			{
				std::lock_guard<std::mutex> lock( m_mutex );
				jobs.swap( m_jobs );
			}
			for( Job& job : jobs )
			{
				job();
			}

			if( m_projectChanged.exchange( false ) )
			{
				RefreshProject();
			}
			if( std::chrono::steady_clock::now() >= m_nextProfilerPoll )
			{
				PollProfiler();
			}

			std::unique_lock<std::mutex> lock( m_mutex );
			m_wake.wait_for( lock, std::chrono::milliseconds( 250 ), [this] { return m_stop || !m_jobs.empty() || m_lost; } );
		}
		Disconnect();
	}

	bool WwiseLink::Connect()
	{
		{
			std::lock_guard<std::mutex> lock( m_mutex );
			m_status.connection = WwiseStatus::Connection::Connecting;
		}
		m_lost = false;
		m_client = std::make_unique<AK::WwiseAuthoringAPI::Client>();
		const bool connected = m_client->Connect( m_options.host.c_str(), m_options.port, [this] {
			m_lost = true;
			m_wake.notify_all();
		}, CONNECT_TIMEOUT_MS );
		if( !connected )
		{
			m_client.reset();
			SetError( "Wwise Authoring is not running, or its WAAPI server is off (Project > User Preferences, port " + std::to_string( m_options.port ) + ")." );
			return false;
		}
		OnConnected();
		return true;
	}

	void WwiseLink::Disconnect()
	{
		if( !m_client )
		{
			return;
		}
		if( !m_lost )
		{
			for( const uint64_t subscription : m_subscriptions )
			{
				std::string result;
				m_client->Unsubscribe( subscription, result, CALL_TIMEOUT_MS );
			}
		}
		m_subscriptions.clear();
		m_client->Disconnect();
		m_client.reset();

		std::lock_guard<std::mutex> lock( m_mutex );
		m_status = WwiseStatus{ WwiseStatus::Connection::Connecting };
		m_jobs.clear();
	}

	void WwiseLink::OnConnected()
	{
		std::string result;
		std::string version;
		if( Call( waapi::core::getInfo, "{}", result, CALL_TIMEOUT_MS ) )
		{
			const Json info = Parse( result );
			const auto it = info.find( "version" );
			if( it != info.end() && it->is_object() )
			{
				version = GetString( *it, "displayName" );
			}
		}

		const auto subscribe = [this]( const char* uri, auto handler ) {
			uint64_t id = 0;
			std::string subscribeResult;
			if( m_client->Subscribe( uri, "{}", [handler]( uint64_t, const AK::WwiseAuthoringAPI::JsonProvider& json ) { handler( Parse( json.GetJsonString() ) ); }, id, subscribeResult, CALL_TIMEOUT_MS ) )
			{
				m_subscriptions.push_back( id );
			}
		};
		subscribe( waapi::core::soundbank::generationDone, [this]( const Json& ) {
			Notify( { WwiseNotice::Kind::GenerationDone, LOG_INFO, "Wwise", "SoundBank generation finished." } );
		} );
		subscribe( waapi::core::log::itemAdded, [this]( const Json& json ) {
			const auto item = json.find( "item" );
			if( item == json.end() || !item->is_object() )
			{
				return;
			}
			const std::string severity = GetString( *item, "severity" );
			const int level = severity.find( "Error" ) != std::string::npos ? LOG_ERROR : severity == "Warning" ? LOG_WARNING : LOG_INFO;
			Notify( { WwiseNotice::Kind::Log, level, "Wwise/" + GetString( json, "channel" ), GetString( *item, "message" ) } );
		} );
		subscribe( waapi::core::project::loaded, [this]( const Json& ) {
			m_projectChanged = true;
			m_wake.notify_all();
		} );
		subscribe( waapi::core::project::postClosed, [this]( const Json& ) {
			m_projectChanged = true;
			m_wake.notify_all();
		} );

		{
			std::lock_guard<std::mutex> lock( m_mutex );
			m_status.connection = WwiseStatus::Connection::Connected;
			m_status.version = version;
			m_status.lastError.clear();
		}
		RefreshProject();
		m_nextProfilerPoll = std::chrono::steady_clock::now();

		const WwiseStatus status = GetStatus();
		std::string message = "Connected to Wwise " + status.version;
		message += status.projectName.empty() ? " with no project open." : ", project " + status.projectName + ".";
		Notify( { WwiseNotice::Kind::Connected, LOG_INFO, "Wwise", message } );
	}

	void WwiseLink::RefreshProject()
	{
		std::string result;
		Json project = Json::object();
		if( Call( waapi::core::getProjectInfo, "{}", result, CALL_TIMEOUT_MS ) )
		{
			project = Parse( result );
		}
		std::lock_guard<std::mutex> lock( m_mutex );
		m_status.projectName = GetString( project, "name" );
		m_status.projectPath = GetString( project, "path" );
		m_status.isLabProject = SameFile( m_status.projectPath, m_options.labProject );
	}

	void WwiseLink::PollProfiler()
	{
		m_nextProfilerPoll = std::chrono::steady_clock::now() + PROFILER_POLL_INTERVAL;

		std::string result;
		if( !Call( waapi::core::remote::getConnectionStatus, "{}", result, CALL_TIMEOUT_MS ) )
		{
			return;
		}
		const Json status = Parse( result );
		const bool connected = GetBool( status, "isConnected" );
		std::string target;
		const auto console = status.find( "console" );
		if( connected && console != status.end() && console->is_object() )
		{
			target = GetString( *console, "appName" );
		}

		bool autoAttach = false;
		{
			std::lock_guard<std::mutex> lock( m_mutex );
			m_status.profilerAttached = connected && target == m_options.appName;
			m_status.profilerTarget = target;
			autoAttach = m_autoAttach && !m_userDetached && !connected && !m_options.appName.empty();
		}

		if( autoAttach )
		{
			const Json args = { { "host", m_options.host }, { "appName", m_options.appName } };
			if( Call( waapi::core::remote::connect, args.dump(), result, ATTACH_TIMEOUT_MS ) )
			{
				Notify( { WwiseNotice::Kind::Log, LOG_INFO, "Wwise", "The Wwise Profiler is connected to this process." } );
				m_nextProfilerPoll = std::chrono::steady_clock::now();
			}
			else
			{
				// Trying again would hold up every other request for the attach timeout, every poll.
				SetError( "The Wwise Profiler could not connect to this process (" + result + "); attaching it automatically is off until it is turned on again." );
				std::lock_guard<std::mutex> lock( m_mutex );
				m_autoAttach = false;
			}
		}
	}
}
