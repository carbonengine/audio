// Copyright © 2026 CCP ehf.

#pragma once

#include <chrono>
#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace AudioLab
{
	class LabApp;
	class LabPage;

	/// A check's steps. Each step runs once a frame, before the scene is sent and CarbonAudio ticks.
	class CheckScript
	{
	public:
		/// Fills detail with what it saw, for the failure message.
		using Condition = std::function<bool( LabApp& app, std::string& detail )>;

		struct Step
		{
			enum class Kind
			{
				Do,
				Wait,
				Expect,
				ExpectFor,
			};

			Kind kind = Kind::Do;
			std::string what;
			std::function<void( LabApp& app )> action;
			Condition condition;
			double seconds = 0.0;
		};

		/// Runs once.
		void Do( std::string what, std::function<void( LabApp& app )> action );
		/// Lets CarbonAudio tick for this long.
		void Wait( double seconds );
		/// Passes as soon as the condition holds; fails when it has not held within the timeout.
		void Expect( std::string what, Condition condition, double timeoutSeconds = 3.0 );
		/// Fails as soon as the condition does not hold during the time.
		void ExpectFor( std::string what, Condition condition, double seconds );

		const std::vector<Step>& GetSteps() const { return m_steps; }

	private:
		std::vector<Step> m_steps;
	};

	struct LabCheck
	{
		/// Short and lowercase; `--check <page>/<name>` runs it.
		std::string name;
		std::string description;
		/// Called on a freshly opened page, without the page's demo scene.
		std::function<void( CheckScript& script )> build;
	};

	struct CheckResult
	{
		std::string page;
		std::string check;
		bool passed = false;
		/// The step that failed and what it saw.
		std::string message;
		double seconds = 0.0;
	};

	/// Runs queued checks one after another, each on a freshly opened page, one step a frame.
	class CheckRunner
	{
	public:
		/// Reads every page's checks once.
		void SetPages( const std::vector<std::unique_ptr<LabPage>>& pages );
		const std::vector<LabCheck>& GetChecks( int page ) const { return m_pages[page].checks; }

		/// Queues "all" checks, every check of a page ("<page>") or one check ("<page>/<check>"); false when none matches.
		bool Queue( const std::string& spec );
		bool IsRunning() const { return m_running || !m_queue.empty(); }
		/// Fails the running check with the reason and drops the queued ones.
		void Stop( const std::string& reason );

		/// Takes the next queued check when none runs; true when the caller has to open its page, at GetRunningPage().
		bool StartNext();
		int GetRunningPage() const { return m_check.page; }
		/// Runs the running check's next step.
		void Step( LabApp& app );

		/// Oldest first.
		const std::vector<CheckResult>& GetResults() const { return m_results; }
		void ClearResults() { m_results.clear(); }

	private:
		struct PageChecks
		{
			std::string id;
			std::vector<LabCheck> checks;
		};

		struct QueuedCheck
		{
			int page = 0;
			LabCheck check;
		};

		void Finish( bool passed, const std::string& message );

		std::vector<PageChecks> m_pages;
		std::deque<QueuedCheck> m_queue;
		bool m_running = false;
		QueuedCheck m_check;
		CheckScript m_script;
		size_t m_step = 0;
		std::chrono::steady_clock::time_point m_checkStarted;
		std::chrono::steady_clock::time_point m_stepStarted;
		std::vector<CheckResult> m_results;
	};
}
