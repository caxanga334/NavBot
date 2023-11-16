#include <extension.h>
#include <ifaces_extern.h>
#include <extplayer.h>
#include <bot/interfaces/base_interface.h>
#include <bot/interfaces/knownentity.h>
#include "basebot.h"

// TO-DO: Add a convar
constexpr auto BOT_UPDATE_INTERVAL = 0.15f;

extern CGlobalVars* gpGlobals;

CBaseBot::CBaseBot(edict_t* edict) : CBaseExtPlayer(edict),
	m_cmd(),
	m_viewangles(0.0f, 0.0f, 0.0f)
{
	m_nextupdatetime = 64;
	m_controller = botmanager->GetBotController(edict);
	m_basecontrol = nullptr;
	m_weaponselect = 0;
}

CBaseBot::~CBaseBot()
{
	if (m_basecontrol)
	{
		delete m_basecontrol;
	}
}

void CBaseBot::PlayerThink()
{
	CBaseExtPlayer::PlayerThink(); // Call base class function first

	Frame(); // Call bot frame

	if (--m_nextupdatetime <= 0)
	{
		m_nextupdatetime = TIME_TO_TICKS(BOT_UPDATE_INTERVAL);

		Update(); // Run period update
	}
}

CBaseBot* CBaseBot::MyBotPointer()
{
	return this;
}

void CBaseBot::Reset()
{
	m_nextupdatetime = 1;

	for (IBotInterface* current = m_head; current != nullptr; current = current->GetNext())
	{
		current->Reset();
	}
}

void CBaseBot::Update()
{
	for (IBotInterface* current = m_head; current != nullptr; current = current->GetNext())
	{
		current->Update();
	}
}

void CBaseBot::Frame()
{
	for (IBotInterface* current = m_head; current != nullptr; current = current->GetNext())
	{
		current->Frame();
	}

	// This needs to be called after the interfaces frame function
	// So any button changes are sent on the same frame
	BuildUserCommand();
}

void CBaseBot::RegisterInterface(IBotInterface* iface)
{
	if (m_head == nullptr) // First on the list
	{
		m_head = iface;
		return;
	}

	IBotInterface* next = m_head;
	while (true)
	{
		if (next->GetNext() == nullptr)
		{
			next->SetNext(iface);
			return;
		}

		next = next->GetNext();
	}
}

void CBaseBot::BuildUserCommand()
{
	int commandnumber = m_cmd.command_number;

	m_cmd.Reset();

	commandnumber++;
	m_cmd.command_number = commandnumber;
	m_cmd.tick_count = gpGlobals->tickcount;
	m_cmd.impulse = 0;
	m_cmd.viewangles = m_viewangles;
	m_cmd.weaponselect = m_weaponselect;

	auto control = GetControlInterface();
	control->ProcessButtons(&m_cmd);
	m_controller->RunPlayerMove(&m_cmd);

	m_weaponselect = 0;
}

IPlayerController* CBaseBot::GetControlInterface()
{
	if (m_basecontrol == nullptr)
	{
		m_basecontrol = new IPlayerController(this);
		RegisterInterface(m_basecontrol);
	}

	return m_basecontrol;
}
