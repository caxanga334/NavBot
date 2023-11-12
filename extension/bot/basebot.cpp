#include "extension.h"
#include "extplayer.h"
#include "basebot.h"

// TO-DO: Add a convar
constexpr auto BOT_UPDATE_INTERVAL = 0.15f;

extern CGlobalVars* gpGlobals;

CBaseBot::CBaseBot(edict_t* edict) : CBaseExtPlayer(edict)
{
	m_nextupdatetime = 64;
}

CBaseBot::~CBaseBot()
{
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
}

void CBaseBot::Update()
{
}

void CBaseBot::Frame()
{
}
