// Copyright © 2026 CCP ehf.

#pragma once

#include "LabScene.h"

#include <string>
#include <vector>

namespace AudioLab
{
	/// What the user selected in the scene, as scene IDs; shared by the scene and the sidebar.
	class Selection
	{
	public:
		explicit Selection( LabScene& scene ) : m_scene( scene ) {}

		const std::vector<int>& GetIDs() const { return m_ids; }
		bool IsEmpty() const { return m_ids.empty(); }
		bool Contains( int id ) const;
		/// How many selected things are of the kind.
		int Count( LabKind kind ) const;
		/// E.g. "3 emitters and a room", or the name of the one thing.
		std::string Describe() const;

		/// Selects only the thing, or nothing for LabScene::NO_ID, and asks for it to be shown.
		void Select( int id );
		/// Adds the thing to the selection, or takes it out when it is there.
		void Toggle( int id );
		/// Adds the thing without asking for it to be shown.
		void Add( int id );
		void Clear() { m_ids.clear(); }
		/// Drops the selected things that are no longer in the scene.
		void Prune();

		/// Removes the selected emitters, rooms and walls from the scene; the listener stays.
		void RemoveFromScene();
		/// Plays or stops every selected emitter.
		void Play( bool play );

		/// Asks the sidebar to show the selection and the object list to scroll to it.
		void RequestShow();
		/// Whether the sidebar should bring the selection to the front, once.
		bool TakeShowRequest();
		/// Whether the object list should scroll to the last selected thing; it clears it once it has.
		bool WantsScroll() const { return m_scrollRequested; }
		void Scrolled() { m_scrollRequested = false; }

	private:
		LabScene& m_scene;
		std::vector<int> m_ids;
		bool m_showRequested = false;
		bool m_scrollRequested = false;
	};
}
