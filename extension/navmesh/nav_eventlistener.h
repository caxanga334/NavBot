// Copy of the SDK's GameEventListener
// some sdk versions are broken

#ifndef NAV_GAME_EVENT_LISTENER_H
#define NAV_GAME_EVENT_LISTENER_H
#ifdef _WIN32
#pragma once
#endif

#include "igameevents.h"
extern IGameEventManager2* gameeventmanager;

// A safer method than inheriting straight from IGameEventListener2.
// Avoids requiring the user to remove themselves as listeners in 
// their deconstructor, and sets the serverside variable based on
// our dll location.
class CNavGameEventListener : public IGameEventListener2
{
public:
	CNavGameEventListener() : m_bRegisteredForEvents(false)
	{
	}

	~CNavGameEventListener()
	{
		StopListeningForAllEvents();
	}

	void ListenForGameEvent(const char* name)
	{
		m_bRegisteredForEvents = true;

#ifdef CLIENT_DLL
		bool bServerSide = false;
#else
		bool bServerSide = true;
#endif
		if (gameeventmanager)
			gameeventmanager->AddListener(this, name, bServerSide);
	}

	void StopListeningForAllEvents()
	{
		// remove me from list
		if (m_bRegisteredForEvents)
		{
			if (gameeventmanager)
				gameeventmanager->RemoveListener(this);
			m_bRegisteredForEvents = false;
		}
	}

	// Intentionally abstract
	virtual void FireGameEvent(IGameEvent* event) = 0;

private:

	// Have we registered for any events?
	bool m_bRegisteredForEvents;
};

#endif

