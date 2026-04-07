#ifndef __NAVBOT_BOT_SHARED_SQUAD_TRY_FORM_TASK_H_
#define __NAVBOT_BOT_SHARED_SQUAD_TRY_FORM_TASK_H_

#include <extension.h>
#include <manager.h>
#include <util/helpers.h>
#include <bot/basebot.h>

/**
 * @brief Utility tasks for forming bot squads.
 * @tparam BT Bot class.
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
	ISquad* squad = bot->GetSquadInterface();

	if (!squad->CanLeadSquads())
	{
		return AITask<BT>::Done("I can't lead squads!");
	}

	int teamwork = std::clamp(bot->GetDifficultyProfile()->GetTeamwork(), 0, 80);

	if (!CBaseBot::s_botrng.GetRandomChance(teamwork))
	{
		return AITask<BT>::Done("Not creating squad (random chance, teamwork skill based).");
	}

	return AITask<BT>::Continue();
}

template<typename BT>
inline TaskResult<BT> CBotSharedTryFormingSquadTask<BT>::OnTaskUpdate(BT* bot)
{
	ISquad* squad = bot->GetSquadInterface();

	if (squad->IsInASquad())
	{
		return AITask<BT>::Done("Already in a squad!");
	}

	botsquadutils::TryFormSquadFunc func{ bot, nullptr, m_maxmembers };
	extmanager->ForEachBot(func); // collect candidates for my squad
	func.CreateSquad();
	return AITask<BT>::Done("Done inviting to squad!");
}

#endif // !__NAVBOT_BOT_SHARED_SQUAD_TRY_FORM_TASK_H_