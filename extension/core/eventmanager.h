#ifndef SMNAV_CORE_GAME_EVENT_MANAGER_H_
#define SMNAV_CORE_GAME_EVENT_MANAGER_H_
#pragma once

#include <string>
#include <vector>

#include <mods/gamemods_shared.h>

#include <igameevents.h>

class IGameEvent;

/*
* Event Manager
* 
* RCBot2 style event listeners
*/


// Interface for received game events
class IEventReceiver
{
public:
	IEventReceiver(const char* eventname, Mods::ModType mod);
	virtual ~IEventReceiver();

	// This function is called when the event this listener wants is fired
	virtual void OnGameEvent(IGameEvent* gameevent);

	// true if the event name matches with the event this receiver wants
	inline bool EventMatches(std::string& name) const { return m_eventname.compare(name) == 0; }

	// true if the OnGameEvent function should be called
	inline bool ShouldNotify(Mods::ModType mod) const { return m_mod == Mods::MOD_ALL || m_mod == mod; }

	// Gets the name of the event this receiver is listening to
	inline const std::string& GetListeningName() const { return m_eventname; }
private:
	std::string m_eventname;
	Mods::ModType m_mod;
};

class EventManager : public IGameEventListener2
{
public:
	EventManager();
	virtual ~EventManager();

	virtual void FireGameEvent(IGameEvent* event) override;

	void Load();
	void Unload();
	void RegisterEventReceiver(IEventReceiver* receiver);

private:
	bool m_loaded;
	bool m_islistening;
	std::vector<IEventReceiver*> m_eventlisteners; // List of registered receivers
};

// Returns a pointer to the global game event manager
EventManager* GetGameEventManager();

#endif // !SMNAV_CORE_GAME_EVENT_MANAGER_H_
