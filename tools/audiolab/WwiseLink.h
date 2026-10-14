// Copyright © 2026 CCP ehf.

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace AK
{
	namespace WwiseAuthoringAPI
	{
		class Client;
	}
}

namespace AudioLab
{
	/// What the lab knows about Wwise Authoring, copied out of the link's thread.
	struct WwiseStatus
	{
		enum class Connection
		{
			Off,
			Connecting,
			Connected,
		};

		Connection connection = Connection::Off;
		/// Wwise Authoring's version, e.g. "v2025.1.10.9233".
		std::string version;
		std::string projectName;
		std::string projectPath;
		/// Whether the open project is the lab's own, which its SoundBanks come from.
		bool isLabProject = false;
		/// Whether the Wwise Profiler is connected to this process.
		bool profilerAttached = false;
		/// The application the Wwise Profiler is connected to, this one or another.
		std::string profilerTarget;
		bool capturing = false;
		std::string lastError;
	};

	/// Something Wwise Authoring reported, handled on the main thread.
	struct WwiseNotice
	{
		enum class Kind
		{
			Connected,
			Disconnected,
			GenerationDone,
			Log,
		};

		Kind kind = Kind::Log;
		/// CCP::LogType, for log notices.
		int severity = 0;
		std::string channel;
		std::string message;
	};

	/// The lab's link to Wwise Authoring over WAAPI. A thread owns the WAAPI client and reconnects when Wwise
	/// restarts; the calls here return at once and results come back through GetStatus() and Drain().
	class WwiseLink
	{
	public:
		struct Options
		{
			std::string host = "127.0.0.1";
			unsigned int port = 8080;
			/// The name Wwise communication gives this process, so the Wwise Profiler can be pointed at it.
			std::string appName;
			std::filesystem::path labProject;
			bool enabled = true;
			/// Attach the Wwise Profiler to this process when it is not connected to anything; turns itself off when an
			/// attach fails.
			bool autoAttachProfiler = true;
		};

		WwiseLink();
		WwiseLink( const WwiseLink& ) = delete;
		WwiseLink& operator=( const WwiseLink& ) = delete;
		~WwiseLink();

		void Start( const Options& options );
		/// Disconnects and joins the thread; call before Blue ends the process.
		void Shutdown();

		void SetEnabled( bool enabled );
		bool IsEnabled() const;
		void SetAutoAttachProfiler( bool enabled );
		bool GetAutoAttachProfiler() const;
		void AttachProfiler();
		void DetachProfiler();
		/// Profiler capture in Wwise Authoring, i.e. what its Profiler layout records.
		void StartCapture();
		void StopCapture();
		/// Generates every SoundBank of the open project for every platform, as F7 in Wwise does.
		void GenerateSoundBanks();

		WwiseStatus GetStatus() const;
		/// Notices since the last call, oldest first.
		std::vector<WwiseNotice> Drain();

	private:
		using Job = std::function<void()>;

		void Run();
		bool Connect();
		void Disconnect();
		void OnConnected();
		void RefreshProject();
		void PollProfiler();
		bool Call( const char* uri, const std::string& args, std::string& result, int timeoutMs );
		void Queue( Job job );
		void Notify( WwiseNotice notice );
		void SetError( const std::string& error );

		Options m_options;
		std::unique_ptr<AK::WwiseAuthoringAPI::Client> m_client;
		std::vector<uint64_t> m_subscriptions;
		std::thread m_thread;

		mutable std::mutex m_mutex;
		std::condition_variable m_wake;
		std::deque<Job> m_jobs;
		std::vector<WwiseNotice> m_notices;
		WwiseStatus m_status;
		bool m_stop = false;
		bool m_enabled = true;
		bool m_autoAttach = true;
		/// Set by the user detaching, so auto-attach does not connect again straight away.
		bool m_userDetached = false;

		std::atomic<bool> m_lost{ false };
		std::atomic<bool> m_projectChanged{ false };
		std::chrono::steady_clock::time_point m_nextProfilerPoll;
	};
}
