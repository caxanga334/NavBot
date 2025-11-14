#ifndef __NAVBOT_BOT_SHARED_HIDE_TASK_H_
#define __NAVBOT_BOT_SHARED_HIDE_TASK_H_

#include <vector>
#include <extension.h>
#include <sdkports/sdk_timers.h>
#include <navmesh/nav_mesh.h>
#include <bot/basebot.h>
#include <bot/basebot_pathcost.h>
#include <bot/bot_shared_utils.h>
#include <bot/interfaces/path/meshnavigator.h>

template <typename BT, typename CT = CBaseBotPathCost>
class CBotSharedHideTask : public AITask<BT>
{
public:
	static bool IsPossible(BT* bot, const float maxDistance, const HidingSpot** hidingspot)
	{
		botsharedutils::HidingSpotCollector collector{ static_cast<CBaseBot*>(bot), maxDistance };
		collector.Execute();
		
		CNavArea* area = collector.GetRandomHidingArea();

		if (!area) { return false; }

		const HidingSpot* spot = area->GetRandomHidingSpot();
		*hidingspot = spot;
		return spot != nullptr;
	}

	CBotSharedHideTask(BT* bot, const HidingSpot* hidingSpot, const float maxHideTime, const bool crouch = true) :
		m_pathcost(bot)
	{
		m_hidingspot = hidingSpot;
		m_hidelength = maxHideTime;
		m_shouldCrouch = crouch;
	}

	TaskResult<BT> OnTaskStart(BT* bot, AITask<BT>* pastTask) override
	{
		m_pathcost.SetRouteType(RouteType::SAFEST_ROUTE);
		const Vector& pos = m_hidingspot->GetPosition();

		if (!m_nav.ComputePathToPosition(bot, pos, m_pathcost))
		{
			return AITask<BT>::Done("No path found to hiding spot!");
		}
		else
		{
			m_nav.StartRepathTimer();
		}

		return AITask<BT>::Continue();
	}

	TaskResult<BT> OnTaskUpdate(BT* bot) override
	{
		if (m_hideTimer.HasStarted())
		{
			bot->GetCombatInterface()->DisableDodging(0.2f);

			if (m_shouldCrouch)
			{
				const CNavArea* area = m_hidingspot->GetArea();

				if (area && !area->HasAttributes(static_cast<int>(NavAttributeType::NAV_MESH_STAND)))
				{
					bot->GetControlInterface()->PressCrouchButton(0.3f);
				}
			}

			if (m_aimTimer.IsElapsed() && !m_aimSpots.empty())
			{
				m_aimTimer.StartRandom(3.0f, 5.0f);
				const Vector& spot = m_aimSpots[CBaseBot::s_botrng.GetRandomInt<std::size_t>(0U, m_aimSpots.size() - 1U)];
				bot->GetControlInterface()->AimAt(spot, IPlayerController::LOOK_SEARCH, 2.5f, "Looking at potential enemy position!");
				bot->GetCombatInterface()->StopLookingAround();
			}

			if (m_hideTimer.IsElapsed())
			{
				return AITask<BT>::Done("Hiding timer elapsed!");
			}

			return AITask<BT>::Continue();
		}

		if (m_nav.NeedsRepath())
		{
			m_nav.ComputePathToPosition(bot, m_hidingspot->GetPosition(), m_pathcost);
			m_nav.StartRepathTimer();
		}

		m_nav.Update(bot);

		return AITask<BT>::Continue();
	}

	bool OnTaskPause(BT* bot, AITask<BT>* nextTask) override
	{
		return false; // destroy this task when paused
	}

	TaskEventResponseResult<BT> OnMoveToSuccess(BT* bot, CPath* path) override
	{
		m_hideTimer.Start(m_hidelength);
		botsharedutils::AimSpotCollector collector{ static_cast<CBaseBot*>(bot) };
		collector.Execute();
		collector.ExtractAimSpots(m_aimSpots);
		bot->GetInventoryInterface()->SelectBestWeapon();
		return AITask<BT>::TryContinue();
	}

	QueryAnswerType ShouldSeekAndDestroy(CBaseBot* me, const CKnownEntity* them) override { return ANSWER_NO; }
	QueryAnswerType ShouldPickup(CBaseBot* me, CBaseEntity* item) override { return ANSWER_NO; }
	QueryAnswerType ShouldHurry(CBaseBot* me) override { return ANSWER_YES; }
	QueryAnswerType ShouldRetreat(CBaseBot* me) override { return ANSWER_NO; }
	QueryAnswerType IsIgnoringMapObjectives(CBaseBot* me) override { return ANSWER_YES; }
	QueryAnswerType ShouldAssistTeammate(CBaseBot* me, CBaseEntity* teammate) override { return ANSWER_NO; }

	const char* GetName() const override { return "Hide"; }
private:
	CT m_pathcost;
	CMeshNavigator m_nav;
	const HidingSpot* m_hidingspot;
	std::vector<Vector> m_aimSpots;
	CountdownTimer m_aimTimer;
	float m_hidelength;
	bool m_shouldCrouch;
	CountdownTimer m_hideTimer;
};


#endif // !__NAVBOT_BOT_SHARED_HIDE_TASK_H_
