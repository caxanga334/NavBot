#include NAVBOT_PCH_FILE
#include "hl1mp_bot.h"

CHL1MPBot::CHL1MPBot(edict_t* edict) :
	CBaseBot(edict)
{
	m_hl1mpsensor = std::make_unique<CHL1MPBotSensor>(this);
	m_hl1mpbehavior = std::make_unique<CHL1MPBotBehavior>(this);
	m_hl1mpmovement = std::make_unique<CHL1MPBotMovement>(this);
	m_hl1mpsquad = std::make_unique<CHL1MPBotSquad>(this);
}

CHL1MPBot::~CHL1MPBot()
{
}
