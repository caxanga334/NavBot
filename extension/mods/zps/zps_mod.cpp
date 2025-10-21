#include NAVBOT_PCH_FILE
#include <bot/zps/zpsbot.h>
#include <navmesh/nav_mesh.h>
#include "zps_mod.h"

CZombiePanicSourceMod::CZombiePanicSourceMod()
{
	ListenForGameEvent("endslate");
	ListenForGameEvent("clientsound");
}

void CZombiePanicSourceMod::FireGameEvent(IGameEvent* event)
{
	if (event)
	{
		const char* name = event->GetName();

#ifdef EXT_DEBUG
		META_CONPRINTF("CZombiePanicSourceMod::FireGameEvent: %s \n", name);
#endif // EXT_DEBUG

		if (V_strcmp(name, "endslate") == 0)
		{
			OnRoundEnd();
			return;
		}
		else if (V_strcmp(name, "clientsound") == 0)
		{
			const char* sound = event->GetString("sound");

			if (V_strcmp(sound, "Round_Starting") == 0)
			{
				m_roundstarttimer.Start(9.0f);
			}
			return;
		}

		CBaseMod::FireGameEvent(event);
	}
}

void CZombiePanicSourceMod::Update()
{
	CBaseMod::Update();

	if (m_roundstarttimer.HasStarted() && m_roundstarttimer.IsElapsed())
	{
		m_roundstarttimer.Invalidate();
		OnRoundStart();
	}
}

CBaseBot* CZombiePanicSourceMod::AllocateBot(edict_t* edict)
{
	return new CZPSBot(edict);
}

void CZombiePanicSourceMod::OnRoundStart()
{
#ifdef EXT_DEBUG
	META_CONPRINT("CZombiePanicSourceMod::OnRoundStart() \n");
#endif // EXT_DEBUG

	CBaseMod::OnRoundStart();
	CNavMesh::NotifyRoundRestart();

	// ZPS gives weapons when the bots joins a team, force a refresh
	IInventory::RefreshInventory func{ 1.0f };
	extmanager->ForEachBot(func);
}

void CZombiePanicSourceMod::OnRoundEnd()
{
#ifdef EXT_DEBUG
	META_CONPRINT("CZombiePanicSourceMod::OnRoundEnd() \n");
#endif // EXT_DEBUG

	CBaseMod::OnRoundEnd();
}
