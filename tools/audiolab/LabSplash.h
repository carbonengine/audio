// Copyright © 2026 CCP ehf.

#pragma once

namespace AudioLab
{
	/// What the lab shows over everything for its first four seconds, like a terminal in ImGui's own font: a boot log
	/// typing out, "AUDIO LAB" decoding from noise and a level meter under it. Space ends it early, and so does any
	/// other key or a click.
	class Splash
	{
	public:
		/// Last in the frame, between ImGui::NewFrame() and ImGui::Render(); nothing once it is over.
		void Draw( float uiScale );
		bool IsOver() const { return m_over; }

	private:
		/// Seconds it has shown, at most a short step a frame, so a slow first frame does not skip the start.
		float m_time = 0.0f;
		bool m_over = false;
	};
}
