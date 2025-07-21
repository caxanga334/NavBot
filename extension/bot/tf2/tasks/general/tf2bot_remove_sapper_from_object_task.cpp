#include NAVBOT_PCH_FILE
#include <bot/interfaces/path/chasenavigator.h>
#include <bot/tf2/tf2bot.h>
#include <mods/tf2/tf2lib.h>
#include <bot/tasks_shared/bot_shared_attack_enemy.h>
#include "tf2bot_remove_sapper_from_object_task.h"

class FindSappedObject
{
public:
	FindSappedObject() :
		object(nullptr), origin(vec3_origin), best(std::numeric_limits<float>::max()), maxrange(0.0f), team(TEAM_UNASSIGNED)
	{
	}

	bool operator()(int index, edict_t* edict, CBaseEntity* entity)
	{
		if (entity)
		{
			int theirteam = TEAM_UNASSIGNED;
			entprops->GetEntProp(index, Prop_Data, "m_iTeamNum", theirteam);
			bool sapper = false;

			if (theirteam == this->team && entprops->GetEntPropBool(index, Prop_Send, "m_bHasSapper", sapper))
			{
				if (sapper)
				{
					const float range = (UtilHelpers::getEntityOrigin(entity) - origin).Length();

					if (range < this->best && range <= this->maxrange)
					{
						this->best = range;
						this->object = entity;
					}
				}
			}

		}

		return true;
	}

	CBaseEntity* object;
	Vector origin;
	float best;
	float maxrange;
	int team;
};


CTF2BotRemoveObjectSapperTask::CTF2BotRemoveObjectSapperTask(CBaseEntity* object) :
	m_nav(CChaseNavigator::SubjectLeadType::DONT_LEAD_SUBJECT), m_object(object)
{
}

bool CTF2BotRemoveObjectSapperTask::IsPossible(CTF2Bot* bot)
{
	return bot->GetInventoryInterface()->GetSapperRemovalWeapon() != nullptr;
}

CBaseEntity* CTF2BotRemoveObjectSapperTask::FindNearestSappedObject(CTF2Bot* bot)
{
	FindSappedObject functor;
	functor.origin = bot->GetAbsOrigin();
	functor.team = static_cast<int>(bot->GetMyTFTeam());
	functor.maxrange = bot->GetSensorInterface()->GetMaxHearingRange();

	UtilHelpers::ForEachEntityOfClassname("obj_*", functor);

	return functor.object;
}

TaskResult<CTF2Bot> CTF2BotRemoveObjectSapperTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	if (randomgen->GetRandomChance(60))
	{
		bot->SendVoiceCommand(TeamFortress2::VoiceCommandsID::VC_SPY);
	}

	return Continue();
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
		CBaseEntity* newObject = CTF2BotRemoveObjectSapperTask::FindNearestSappedObject(bot);

		if (newObject)
		{
			m_object = newObject;
			return Continue();
		}

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
