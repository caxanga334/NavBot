#include <bot/basebot.h>
#include "base_interface.h"

IBotInterface::IBotInterface(CBaseBot* bot)
{
	m_bot = bot;
	m_next = nullptr;

	bot->RegisterInterface(this);
}