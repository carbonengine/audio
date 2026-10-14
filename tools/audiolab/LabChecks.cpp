// Copyright © 2026 CCP ehf.

#include "LabChecks.h"

#include "BlueBoot.h"
#include "LabPage.h"

namespace AudioLab
{
	void CheckScript::Do( std::string what, std::function<void( LabApp& app )> action )
	{
		Step step;
		step.kind = Step::Kind::Do;
		step.what = std::move( what );
		step.action = std::move( action );
		m_steps.push_back( std::move( step ) );
	}

	void CheckScript::Wait( double seconds )
	{
		Step step;
		step.kind = Step::Kind::Wait;
		step.what = "wait";
		step.seconds = seconds;
		m_steps.push_back( std::move( step ) );
	}

	void CheckScript::Expect( std::string what, Condition condition, double timeoutSeconds )
	{
		Step step;
		step.kind = Step::Kind::Expect;
		step.what = std::move( what );
		step.condition = std::move( condition );
		step.seconds = timeoutSeconds;
		m_steps.push_back( std::move( step ) );
	}

	void CheckScript::ExpectFor( std::string what, Condition condition, double seconds )
	{
		Step step;
		step.kind = Step::Kind::ExpectFor;
		step.what = std::move( what );
		step.condition = std::move( condition );
		step.seconds = seconds;
		m_steps.push_back( std::move( step ) );
	}

	void CheckRunner::SetPages( const std::vector<std::unique_ptr<LabPage>>& pages )
	{
		m_pages.clear();
		for( const auto& page : pages )
		{
			m_pages.push_back( { page->GetID(), page->GetChecks() } );
		}
	}

	bool CheckRunner::Queue( const std::string& spec )
	{
		const size_t slash = spec.find( '/' );
		const std::string pageID = spec.substr( 0, slash );
		const std::string checkName = slash == std::string::npos ? std::string() : spec.substr( slash + 1 );
		bool queued = false;
		for( size_t page = 0; page < m_pages.size(); ++page )
		{
			if( pageID != "all" && pageID != m_pages[page].id )
			{
				continue;
			}
			for( const LabCheck& check : m_pages[page].checks )
			{
				if( checkName.empty() || checkName == check.name )
				{
					m_queue.push_back( { static_cast<int>( page ), check } );
					queued = true;
				}
			}
		}
		return queued;
	}

	void CheckRunner::Stop( const std::string& reason )
	{
		m_queue.clear();
		if( m_running )
		{
			Finish( false, reason );
		}
	}

	bool CheckRunner::StartNext()
	{
		if( m_running || m_queue.empty() )
		{
			return false;
		}
		m_check = std::move( m_queue.front() );
		m_queue.pop_front();
		m_script = CheckScript();
		m_check.check.build( m_script );
		m_running = true;
		m_step = 0;
		m_checkStarted = std::chrono::steady_clock::now();
		m_stepStarted = m_checkStarted;
		return true;
	}

	void CheckRunner::Step( LabApp& app )
	{
		if( !m_running )
		{
			return;
		}
		const std::vector<CheckScript::Step>& steps = m_script.GetSteps();
		if( m_step >= steps.size() )
		{
			Finish( true, std::string() );
			return;
		}

		// One step a frame, so every step sees CarbonAudio ticked after the one before.
		const auto now = std::chrono::steady_clock::now();
		const CheckScript::Step& step = steps[m_step];
		const double elapsed = std::chrono::duration<double>( now - m_stepStarted ).count();
		bool done = false;
		std::string detail;
		switch( step.kind )
		{
		case CheckScript::Step::Kind::Do:
			step.action( app );
			done = true;
			break;
		case CheckScript::Step::Kind::Wait:
			done = elapsed >= step.seconds;
			break;
		case CheckScript::Step::Kind::Expect:
			done = step.condition( app, detail );
			if( !done && elapsed >= step.seconds )
			{
				Finish( false, step.what + ( detail.empty() ? "" : ": " + detail ) );
				return;
			}
			break;
		case CheckScript::Step::Kind::ExpectFor:
			if( !step.condition( app, detail ) )
			{
				Finish( false, step.what + ( detail.empty() ? "" : ": " + detail ) );
				return;
			}
			done = elapsed >= step.seconds;
			break;
		}
		if( done )
		{
			++m_step;
			m_stepStarted = now;
		}
	}

	void CheckRunner::Finish( bool passed, const std::string& message )
	{
		CheckResult result;
		result.page = m_pages[m_check.page].id;
		result.check = m_check.check.name;
		result.passed = passed;
		result.message = message;
		result.seconds = std::chrono::duration<double>( std::chrono::steady_clock::now() - m_checkStarted ).count();
		AddLogLine( passed ? LOG_NOTICE : LOG_ERROR, "Check", ( passed ? "PASS " : "FAIL " ) + result.page + "/" + result.check + ( passed ? "" : ": " + message ) );
		m_results.push_back( std::move( result ) );
		m_running = false;
	}
}
