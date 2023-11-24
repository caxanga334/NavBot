#include <extension.h>
#include <manager.h>
#include <mods/basemod.h>
#include "eventmanager.h"

extern CExtManager* extmanager;
extern IGameEventManager2* gameeventmanager;

EventManager g_EventManager;

IEventReceiver::IEventReceiver(const char* eventname, Mods::ModType mod) :
	m_eventname(eventname)
{
	m_mod = mod;
}

IEventReceiver::~IEventReceiver()
{
}

void IEventReceiver::OnGameEvent(IGameEvent* gameevent)
{
#ifdef SMNAV_DEBUG
	smutils->LogError(myself, "You forgot to override IEventReceiver::OnGameEvent for event \"%s\" mod '%i'", m_eventname.c_str(), static_cast<int>(m_mod));
#endif // SMNAV_DEBUG
}

#ifdef SMNAV_DEBUG
class CDebugEventReceiver : public IEventReceiver
{
public:
	CDebugEventReceiver(const char* eventname, Mods::ModType mod) : IEventReceiver(eventname, mod) {}
	virtual ~CDebugEventReceiver() {}

	virtual void OnGameEvent(IGameEvent* gameevent) override;
};

void CDebugEventReceiver::OnGameEvent(IGameEvent* gameevent)
{
	rootconsole->ConsolePrint("CDebugEventReceiver::OnGameEvent");
}

#endif // SMNAV_DEBUG

EventManager::EventManager() :
	m_loaded(false),
	m_islistening(false)
{
	m_eventlisteners.reserve(64);
}

EventManager::~EventManager()
{
}

void EventManager::FireGameEvent(IGameEvent* gameevent)
{
	if (m_loaded == false || m_islistening == false)
	{
		return;
	}

	// The engine accepts NULL events, this is needed to prevent crashes
	if (gameevent == nullptr)
	{
		return;
	}

	auto szEventName = gameevent->GetName();

	// Invalid event name
	if (szEventName == nullptr || szEventName[0] == '\0')
	{
		return;
	}

#ifdef SMNAV_DEBUG
	smutils->LogMessage(myself, "EventManager::FireGameEvent -- \"%s\"", szEventName);
#endif // SMNAV_DEBUG

	std::string name(szEventName);
	auto currentmod = extmanager->GetMod()->GetModType();

	for (auto& receiver : m_eventlisteners)
	{
		if (receiver->ShouldNotify(currentmod) && receiver->EventMatches(name))
		{
			receiver->OnGameEvent(gameevent);
		}
	}
}

void EventManager::Load()
{
	m_loaded = true;

#ifdef SMNAV_DEBUG
	RegisterEventReceiver(new CDebugEventReceiver("player_spawn", Mods::MOD_ALL));
#endif // SMNAV_DEBUG

	// tell manager we're ready for receiving game events
	extmanager->NotifyRegisterGameEvents();
}

void EventManager::Unload()
{
	m_loaded = false;

	if (m_islistening == true)
	{
		gameeventmanager->RemoveListener(this);
		m_islistening = false;
	}

	for (auto receiver : m_eventlisteners)
	{
		delete receiver;
	}

	m_eventlisteners.clear();
}

void EventManager::RegisterEventReceiver(IEventReceiver* listener)
{
	auto name = listener->GetListeningName().c_str();

	if (gameeventmanager->FindListener(this, name) == false)
	{
		if (gameeventmanager->AddListener(this, name, true) == false)
		{
			smutils->LogError(myself, "Failed to listen for event \"%s\"!", name);
			return;
		}
#ifdef SMNAV_DEBUG
		else
		{
			smutils->LogMessage(myself, "Added event receiver for \"%s\".", name);
		}
#endif // SMNAV_DEBUG

	}

	m_islistening = true;
	m_eventlisteners.push_back(listener);
}

EventManager* GetGameEventManager()
{
	return &g_EventManager;
}

