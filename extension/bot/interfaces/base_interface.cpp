#include NAVBOT_PCH_FILE
#include <extension.h>
#include <bot/basebot.h>
#include "base_interface.h"

IBotInterface::IBotInterface(CBaseBot* bot)
{
	m_bot = bot;

	bot->RegisterInterface(this);
}