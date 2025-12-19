#include NAVBOT_PCH_FILE
#include <navmesh/nav_mesh.h>
#include <mods/dods/nav/dods_nav_area.h>
#include "dodsbot.h"
#include "dodsbot_pathprocessor.h"

CDoDSBotPathProcessor::CDoDSBotPathProcessor(CDoDSBot* bot) :
	IPathProcessor(bot)
{
	m_bombArea = nullptr;
}

void CDoDSBotPathProcessor::Reset()
{
	IPathProcessor::Reset();
	m_bombArea = nullptr;
	m_delayedUpdate.Invalidate();
}

void CDoDSBotPathProcessor::Update()
{
	IPathProcessor::Update();

	if (m_delayedUpdate.IsElapsed())
	{
		m_delayedUpdate.Start(1.0f);

		if (m_bombArea)
		{
			const CDODSNavArea* area = static_cast<const CDODSNavArea*>(m_bombArea);

			if (area->WasBombed() || !area->CanPlantBomb())
			{
				m_bombArea = nullptr;
			}
		}
	}
}

void CDoDSBotPathProcessor::OnAreaBlockedByBomb(const CNavArea* area)
{
	if (!m_bombArea)
	{
		SetFireEvent(true); // notify behavior
		m_bombArea = area;
	}
}
