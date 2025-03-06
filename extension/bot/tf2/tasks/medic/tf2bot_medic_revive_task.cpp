#include <vector>
#include <limits>

#include <extension.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <sdkports/sdk_traces.h>
#include <entities/tf2/tf_entities.h>
#include <bot/tf2/tf2bot.h>
#include <mods/tf2/tf2lib.h>
#include "tf2bot_medic_revive_task.h"

#undef max
#undef min
#undef clamp

static bool IsLineOfHealClearToMarker(const Vector& from, const Vector& to, CBaseEntity* medic, CBaseEntity* marker)
{
	trace::CTraceFilterSimple filter(medic, COLLISION_GROUP_NONE);
	trace_t tr;
	trace::line(from, to, MASK_SHOT, &filter, tr);

	if (!tr.DidHit())
	{
		return true;
	}

	if (tr.m_pEnt == marker)
	{
		return true;
	}

	return tr.startsolid || tr.fraction < 1.0f;
}

CTF2BotMedicReviveTask::CTF2BotMedicReviveTask(CBaseEntity* marker) :
	m_marker(marker), m_aimpos(0.0f, 0.0f, 0.0f), m_goal(0.0f, 0.0f, 0.0f)
{
}

TaskResult<CTF2Bot> CTF2BotMedicReviveTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	if (m_marker.Get() == nullptr)
	{
		return Done("Revive marker is invalid!");
	}

	tfentities::HTFBaseEntity entity(m_marker.Get());
	m_goal = entity.GetAbsOrigin();

	CTF2BotPathCost cost(bot, SAFEST_ROUTE);
	if (!m_nav.ComputePathToPosition(bot, m_goal, cost))
	{
		return Done("Failed to build path to revive marker!");
	}
	
	m_aimpos = m_goal;
	m_aimpos.z += marker_aimpos_z_offset();

	bot->GetControlInterface()->ReleaseAttackButton();
	m_repathTimer.Start(1.2f);

	CBaseEntity* medigun = bot->GetWeaponOfSlot(static_cast<int>(TeamFortress2::TFWeaponSlot::TFWeaponSlot_Secondary));

	if (medigun)
	{
		bot->SelectWeapon(medigun);
	}

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotMedicReviveTask::OnTaskUpdate(CTF2Bot* bot)
{
	if (m_marker.Get() == nullptr)
	{
		return Done("Revive marker is gone.");
	}

	/*
	* TO-DO:
	* Once the medigun beam is attached to the marker entity, deploy shield if possible and look towards the nearest visible threat
	*/

	float rage = 0.0f;
	entprops->GetEntPropFloat(bot->GetIndex(), Prop_Send, "m_flRageMeter", rage);

	if (rage > 0.99f)
	{
		bot->GetControlInterface()->PressSpecialAttackButton();
	}

	Vector from = bot->GetEyeOrigin();

	if (bot->GetRangeTo(m_aimpos) < marker_heal_range() && IsLineOfHealClearToMarker(from, m_aimpos, bot->GetEntity(), m_marker.Get()))
	{
		/*
		* Bot is near the marker, look at it and starting reviving
		*/

		auto control = bot->GetControlInterface();
		bot->GetMovementInterface()->Stop();
		control->AimAt(m_aimpos, IPlayerController::LOOK_CRITICAL, 1.0f, "Looking at revive marker to revive teammate!");

		if (bot->IsLookingTowards(m_aimpos, 0.97f))
		{
			control->PressAttackButton();
		}
	}
	else
	{
		if (m_repathTimer.IsElapsed())
		{
			m_repathTimer.Start(1.2f);

			CTF2BotPathCost cost(bot, SAFEST_ROUTE);
			if (!m_nav.ComputePathToPosition(bot, m_goal, cost))
			{
				return Done("Failed to build path to revive marker!");
			}
		}

		bot->GetControlInterface()->ReleaseAttackButton();
		m_nav.Update(bot);
	}

	return Continue();
}
