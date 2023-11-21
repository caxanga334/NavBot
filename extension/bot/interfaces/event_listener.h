#ifndef SMNAV_BOT_EVENT_LISTENER_H_
#define SMNAV_BOT_EVENT_LISTENER_H_
#pragma once

#include <vector>

// Interface for receiving events
class IEventListener
{
public:

	// Gets a vector containing all event listeners
	virtual std::vector<IEventListener*> *GetListenerVector() { return nullptr; }
	// For linked list event listeners
	virtual IEventListener* GetNextListener() { return nullptr; }

	virtual void OnTestEventPropagation();

};

inline void IEventListener::OnTestEventPropagation()
{
	auto vec = GetListenerVector();

	if (vec)
	{
		for (auto listener : *vec)
		{
			listener->OnTestEventPropagation();
		}
	}

	auto next = GetNextListener();

	if (next)
	{
		next->OnTestEventPropagation();
	}
}


#endif // !SMNAV_BOT_EVENT_LISTENER_H_

