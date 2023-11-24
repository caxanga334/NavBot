#ifndef SMNAV_EXT_EVENT_LISTENER_H_
#define SMNAV_EXT_EVENT_LISTENER_H_
#pragma once

#include <igameevents.h>

extern IGameEventManager2* gameeventmanager;

// Extension port of the game dll CGameEventListener

// Derive from this class to listen for game events
class CEventListenerHelper : public IGameEventListener2
{
public:
	CEventListenerHelper() : m_isListeningForEvents(false) {}
	virtual ~CEventListenerHelper()
	{
		StopListeningForAllEvents();
	}

	inline void ListenForGameEvent(const char* name)
	{
		if (gameeventmanager->FindListener(this, name) == false)
		{
			if (gameeventmanager->AddListener(this, name, true) == true)
			{
				m_isListeningForEvents = true;
			}
		}
	}

	inline void StopListeningForAllEvents()
	{
		if (m_isListeningForEvents == true)
		{
			gameeventmanager->RemoveListener(this);
			m_isListeningForEvents = false;
		}
	}

	// Derived classes implement this to receive game events

	virtual void FireGameEvent(IGameEvent* event) = 0;

private:
	bool m_isListeningForEvents;
};

#endif // !SMNAV_EXT_EVENT_LISTENER_H_

