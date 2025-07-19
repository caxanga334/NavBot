#include NAVBOT_PCH_FILE
#include <bot/interfaces/path/chasenavigator.h>
#include <bot/tf2/tf2bot.h>
#include <mods/tf2/tf2lib.h>
#include <bot/tasks_shared/bot_shared_attack_enemy.h>
#include "tf2bot_remove_sapper_from_object_task.h"

CTF2BotRemoveObjectSapperTask::CTF2BotRemoveObjectSapperTask(CBaseEntity* object) :
	m_nav(CChaseNavigator::SubjectLeadType::DONT_LEAD_SUBJECT), m_object(object)
{
}

bool CTF2BotRemoveObjectSapperTask::IsPossible(CTF2Bot* bot)
{
	return bot->GetInventoryInterface()->GetSapperRemovalWeapon() != nullptr;
}

TaskResult<CTF2Bot> CTF2BotRemoveObjectSapperTask::OnTaskUpdate(CTF2Bot* bot)
{
	CBaseEntity* object = m_object.Get();

	if (!object)
	{
		return Done("Object is gone!");
	}

	if (!tf2lib::IsBuildingSapped(object))
	{
		return Done("Sapper removed!");
	}

	const CKnownEntity* threat = bot->GetSensorInterface()->GetPrimaryKnownThreat(true);

	if (threat && threat->IsPlayer())
	{
		if (tf2lib::GetPlayerClassType(threat->GetIndex()) == TeamFortress2::TFClassType::TFClass_Spy)
		{
			return PauseFor(new CBotSharedAttackEnemyTask<CTF2Bot, CTF2BotPathCost>(bot), "Attacking visible spy!");
		}
	}

	CTF2BotPathCost cost(bot, FASTEST_ROUTE);
	m_nav.Update(bot, object, cost);

	const CTF2BotWeapon* sapperRemover = bot->GetInventoryInterface()->GetSapperRemovalWeapon();

	if (!sapperRemover)
	{
		return Done("Missing weapon capable of removing sappers!");
	}

	const CTF2BotWeapon* activeWeapon = bot->GetInventoryInterface()->GetActiveTFWeapon();

	if (activeWeapon != sapperRemover)
	{
		bot->SelectWeapon(sapperRemover->GetEntity());
	}

	if ((bot->GetEyeOrigin() - UtilHelpers::getWorldSpaceCenter(object)).Length() <= 128.0f)
	{
		bot->GetControlInterface()->AimAt(UtilHelpers::getWorldSpaceCenter(object), IPlayerController::LOOK_SUPPORT, 0.5f, "Looking at object to remove sapper!");
		bot->GetControlInterface()->PressAttackButton();
	}

	return Continue();
}
