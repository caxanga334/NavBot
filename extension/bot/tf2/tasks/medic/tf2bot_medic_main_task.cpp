#include NAVBOT_PCH_FILE
#include <vector>
#include <limits>

#include <extension.h>
#include <util/helpers.h>
#include <bot/tf2/tf2bot.h>
#include <mods/tf2/teamfortress2mod.h>
#include <mods/tf2/tf2lib.h>
#include <entities/tf2/tf_entities.h>
#include <bot/tf2/tasks/scenario/tf2bot_map_ctf.h>
#include "tf2bot_medic_retreat_task.h"
#include "tf2bot_medic_revive_task.h"
#include "tf2bot_medic_heal_task.h"
#include "tf2bot_medic_medieval_task.h"
#include "tf2bot_medic_main_task.h"

#undef max
#undef min
#undef clamp

AITask<CTF2Bot>* CTF2BotMedicMainTask::InitialNextTask(CTF2Bot* bot)
{
	if (CTeamFortress2Mod::GetTF2Mod()->IsPlayingMedievalMode())
	{
		return new CTF2BotMedicMedievalTask;
	}

	return new CTF2BotMedicHealTask;
}

TaskResult<CTF2Bot> CTF2BotMedicMainTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	return Continue();
}

TaskResult<CTF2Bot> CTF2BotMedicMainTask::OnTaskUpdate(CTF2Bot* bot)
{
	return Continue();
}

TaskResult<CTF2Bot> CTF2BotMedicMainTask::OnTaskResume(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	return Continue();
}

TaskEventResponseResult<CTF2Bot> CTF2BotMedicMainTask::OnFlagTaken(CTF2Bot* bot, CBaseEntity* player)
{
	if (bot->GetEntity() == player)
	{
		auto mod = CTeamFortress2Mod::GetTF2Mod();

		if (mod->GetCurrentGameMode() == TeamFortress2::GameModeType::GM_CTF)
		{
			return TryPauseFor(new CTF2BotCTFDeliverFlagTask, PRIORITY_HIGH, "I got the flag, delivering it!");
		}
	}

	return TryContinue();
}

QueryAnswerType CTF2BotMedicMainTask::IsReady(CBaseBot* me)
{
	CTF2Bot* tf2bot = static_cast<CTF2Bot*>(me);
	CBaseEntity* medigun = tf2bot->GetWeaponOfSlot(static_cast<int>(TeamFortress2::TFWeaponSlot::TFWeaponSlot_Secondary));

	if (medigun && tf2lib::GetMedigunUberchargePercentage(gamehelpers->EntityToBCompatRef(medigun)) > 0.99f)
	{
		return ANSWER_YES;
	}

	return ANSWER_NO;
}
