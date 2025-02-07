#include <array>
#include <extension.h>
#include <sdkports/sdk_traces.h>
#include <bot/blackmesa/bmbot.h>
#include "bmbot_deploy_tripmines.h"

CBlackMesaBotDeployTripminesTask::CBlackMesaBotDeployTripminesTask(const Vector& pos)
{
	m_wallPosition = pos;
	m_timeout = -1.0f;
}

bool CBlackMesaBotDeployTripminesTask::IsPossible(CBlackMesaBot* bot, Vector& wallPos)
{
	if (bot->GetInventoryInterface()->HasWeapon("weapon_tripmine") && bot->GetAmmoOfIndex(static_cast<int>(blackmesa::BMAmmoIndex::Ammo_Tripmines)) > 0)
	{
		Vector start = bot->GetAbsOrigin();

		std::array<QAngle, 4> angles{ {
			{0.0f, 0.0f, 0.0f},
			{0.0f, 90.0f, 0.0f},
			{0.0f, 180.0f, 0.0f},
			{0.0f, 270.0f, 0.0f}
			} };

		bool foundWall = false;
		constexpr auto WALL_DIST = 400.0f;

		for (auto& angle : angles)
		{
			Vector dir;
			AngleVectors(angle, &dir);
			dir.NormalizeInPlace();

			Vector end = start + (dir * WALL_DIST);

			trace_t tr;
			trace::line(start, end, MASK_SHOT, bot->GetEntity(), COLLISION_GROUP_NONE, tr);

			if (tr.DidHitWorld())
			{
				foundWall = true;
				wallPos = tr.endpos;
				break;
			}
		}

		return foundWall;
	}

	return false;
}

TaskResult<CBlackMesaBot> CBlackMesaBotDeployTripminesTask::OnTaskStart(CBlackMesaBot* bot, AITask<CBlackMesaBot>* pastTask)
{
	CBaseEntity* tripmine = bot->GetInventoryInterface()->GetWeaponEntity("weapon_tripmine");

	if (tripmine == nullptr)
	{
		return Done("No tripmines to deploy!");
	}

	bot->SelectWeapon(tripmine);

	m_timeout = gpGlobals->curtime + CBaseBot::s_botrng.GetRandomReal<float>(8.0f, 16.0f);

	return Continue();
}

TaskResult<CBlackMesaBot> CBlackMesaBotDeployTripminesTask::OnTaskUpdate(CBlackMesaBot* bot)
{
	if (m_timeout < gpGlobals->curtime)
	{
		return Done("Timed out!");
	}

	bot->GetMovementInterface()->MoveTowards(m_wallPosition);
	bot->GetControlInterface()->AimAt(m_wallPosition, IPlayerController::LOOK_VERY_IMPORTANT, 0.5f, "Looking at wall to deploy tripmine.");

	const float range = bot->GetRangeTo(m_wallPosition);

	if (range <= 128.0f)
	{
		bot->GetControlInterface()->PressCrouchButton();
	}

	if (range <= 64.0f && bot->GetControlInterface()->IsAimOnTarget())
	{
		bot->GetControlInterface()->PressAttackButton();
		return Done("Tripmine deployed!");
	}

	return Continue();
}
