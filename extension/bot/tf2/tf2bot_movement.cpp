#include <extension.h>
#include <ifaces_extern.h>
#include <bot/tf2/tf2bot.h>
#include <mods/tf2/tf2lib.h>
#include "tf2bot_movement.h"

CTF2BotMovement::CTF2BotMovement(CBaseBot* bot) : IMovement(bot)
{
	m_doublejumptimer = -1;
}

CTF2BotMovement::~CTF2BotMovement()
{
}

void CTF2BotMovement::Reset()
{
	m_doublejumptimer = -1;
	IMovement::Reset();
}

void CTF2BotMovement::Frame()
{
	if (m_doublejumptimer >= 0)
	{
		m_doublejumptimer--;

		if (m_doublejumptimer == 0)
		{
			Jump();
		}
	}

	IMovement::Frame();
}

bool CTF2BotMovement::CanDoubleJump() const
{
	auto cls = tf2lib::GetPlayerClassType(GetBot()->GetIndex());

	if (cls == TeamFortress2::TFClass_Scout)
	{
		return true;
	}

	return false;
}

void CTF2BotMovement::DoDoubleJump()
{
	Jump();
	m_doublejumptimer = TIME_TO_TICKS(0.5f);
}
