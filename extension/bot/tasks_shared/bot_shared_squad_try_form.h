#ifndef __NAVBOT_BOT_SHARED_SQUAD_TRY_FORM_TASK_H_
#define __NAVBOT_BOT_SHARED_SQUAD_TRY_FORM_TASK_H_

#include <extension.h>
#include <manager.h>
#include <util/helpers.h>
#include <bot/basebot.h>

/**
 * @brief 
 * @tparam BT 
 */
template <typename BT>
class CBotSharedTryFormingSquadTask : public AITask<BT>
{
public:
	CBotSharedTryFormingSquadTask(int maxmembers = 5) :
		m_maxmembers(maxmembers)
	{
	}

	TaskResult<BT> OnTaskStart(BT* bot, AITask<BT>* pastTask) override;
	TaskResult<BT> OnTaskUpdate(BT* bot) override;

	const char* GetName() const override { return "TryFormingSquad"; }
private:
	int m_maxmembers;
};

template<typename BT>
inline TaskResult<BT> CBotSharedTryFormingSquadTask<BT>::OnTaskStart(BT* bot, AITask<BT>* pastTask)
{
	if (CBaseBot::s_botrng.GetRandomChance(33))
	{
		return AITask<BT>::Done("Not creating squad (random chance).");
	}

	int teamwork = std::clamp(bot->GetDifficultyProfile()->GetTeamwork(), 0, 80);

	if (!CBaseBot::s_botrng.GetRandomChance(teamwork))
	{
		return AITask<BT>::Done("Not creating squad (random chance, teamwork skill based).");
	}

	ISquad* squad = bot->GetSquadInterface();

	squad->CreateSquad(nullptr);
	
	if (!squad->IsSquadLeader())
	{
		return AITask<BT>::Done("Failed to create squad!");
	}



	return AITask<BT>::Continue();
}

template<typename BT>
inline TaskResult<BT> CBotSharedTryFormingSquadTask<BT>::OnTaskUpdate(BT* bot)
{
	ISquad* squad = bot->GetSquadInterface();

	ISquad::InviteBotsToSquadFunc func{ bot, m_maxmembers };

	extmanager->ForEachBot(func);

	// No members were added
	if (squad->GetSquad()->GetMembersCount() == 0U)
	{
		squad->LeaveSquad();
	}

	return AITask<BT>::Done("Done inviting to squad!");
}

#endif // !__NAVBOT_BOT_SHARED_SQUAD_TRY_FORM_TASK_H_