#include NAVBOT_PCH_FILE
#include <extension.h>
#include "pluginbot.h"

CPluginBot::CPluginBot(edict_t* entity) : CBaseBot(entity)
{
	m_pluginbehavior = std::make_unique<CPluginBotBehavior>(this);
	m_pluginmovement = std::make_unique<CPluginBotMovement>(this);
	m_runplayercommands = false;
}

CPluginBot::~CPluginBot()
{
}
