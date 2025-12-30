#include NAVBOT_PCH_FILE
#include <extension.h>
#include <mods/dods/dods_shareddefs.h>
#include <mods/dods/nav/dods_nav_mesh.h>
#include <mods/dods/nav/dods_nav_area.h>
#include "dodsbot.h"
#include "dodsbot_movement.h"

CDoDSBotMovement::CDoDSBotMovement(CBaseBot* bot) :
	IMovement(bot)
{
	m_goProne = false;
}

CDoDSBotMovement::~CDoDSBotMovement()
{
}

void CDoDSBotMovement::Reset()
{
	IMovement::Reset();

	m_goProne = false;
	m_unProneTimer.Invalidate();
}

void CDoDSBotMovement::Update()
{
	IMovement::Update();

	CDoDSBot* bot = GetBot<CDoDSBot>();

	if (m_goProne)
	{
		if (!bot->IsProne())
		{
			bot->GetControlInterface()->PressProneButton();
			m_unProneTimer.Start(1.0f);
		}
		else
		{
			m_goProne = false;
		}
	}

	if (m_unProneTimer.HasStarted() && m_unProneTimer.IsElapsed())
	{
		if (bot->IsProne())
		{
			bot->GetControlInterface()->PressProneButton();
			m_unProneTimer.Start(1.0f);
		}
		else
		{
			m_unProneTimer.Invalidate();
		}
	}
}

void CDoDSBotMovement::OnNavAreaChanged(CNavArea* oldArea, CNavArea* newArea)
{
	CDoDSNavArea* area = static_cast<CDoDSNavArea*>(newArea);

	if (area->HasDoDAttributes(CDoDSNavArea::DoDNavAttributes::DODNAV_REQUIRES_PRONE))
	{
		GetBot<CDoDSBot>()->GetControlInterface()->PressProneButton();
		m_unProneTimer.Start(1.0f);
		m_goProne = true;
	}
}
