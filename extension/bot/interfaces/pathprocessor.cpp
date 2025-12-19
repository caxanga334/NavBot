#include NAVBOT_PCH_FILE
#include <bot/basebot.h>
#include "pathprocessor.h"

IPathProcessor::IPathProcessor(CBaseBot* bot) :
	IBotInterface(bot)
{
	m_fireEvent = false;
}

void IPathProcessor::Reset()
{
	m_fireEvent = false;
}

void IPathProcessor::Update()
{
	CBaseBot* bot = GetBot<CBaseBot>();

	if (ShouldFireEvents())
	{
		SetFireEvent(false);
		bot->OnPathStatusChanged();
	}
}

void IPathProcessor::Frame()
{
}
