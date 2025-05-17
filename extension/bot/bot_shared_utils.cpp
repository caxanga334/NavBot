#include <extension.h>
#include <util/helpers.h>
#include <util/prediction.h>
#include <util/entprops.h>
#include "basebot.h"
#include "bot_shared_utils.h"

botsharedutils::AimSpotCollector::AimSpotCollector(CBaseBot* bot) :
	INavAreaCollector(bot->GetLastKnownNavArea(), bot->GetSensorInterface()->GetMaxVisionRange(), true, true, false), 
	m_vecOffset1(0.0f, 0.0f, 32.0f), m_vecOffset2(0.0f, 0.0f, 60.0f)
{
	m_aimOrigin = bot->GetEyeOrigin();
	m_filter.SetPassEntity(bot->GetEntity());
}

bool botsharedutils::AimSpotCollector::ShouldCollect(CNavArea* area)
{
	bool visibleneighbors = true;

	if (BlocksLOS(area->GetCenter() + m_vecOffset1) && BlocksLOS(area->GetCenter() + m_vecOffset2))
	{
		return false; // center is not visible
	}

	std::vector<unsigned int> neighborsIDs;

	area->ForEachConnectedArea([this, &visibleneighbors, &neighborsIDs](CNavArea* connectedArea) {
		neighborsIDs.push_back(connectedArea->GetID());

		bool nolos = (this->BlocksLOS(connectedArea->GetCenter() + m_vecOffset1) && this->BlocksLOS(connectedArea->GetCenter() + m_vecOffset2));

		// connected area can't be seen
		if (visibleneighbors && nolos)
		{
			visibleneighbors = false;
		}
	});

	for (unsigned int& id : neighborsIDs)
	{
		if (m_checkedAreas.count(id) > 0)
		{
			return false; // already check, skip
		}
	}

	if (!visibleneighbors)
	{
		for (unsigned int& id : neighborsIDs)
		{
			m_checkedAreas.insert(id);
		}

		return true; // this area contains connected areas that aren't visible, an enemy may arrive from it
	}

	return false;
}

void botsharedutils::AimSpotCollector::OnDone()
{
	for (CNavArea* area : GetCollectedAreas())
	{
		m_aimSpots.push_back(area->GetCenter() + m_vecOffset1);
	}
}

bool botsharedutils::AimSpotCollector::BlocksLOS(Vector endpos)
{
	trace_t tr;
	trace::line(m_aimOrigin, endpos, MASK_VISIBLE, &m_filter, tr);
	return tr.fraction < 1.0f;
}

botsharedutils::TraceFilterVisibleSimple::TraceFilterVisibleSimple() :
	trace::CTraceFilterSimple(nullptr, COLLISION_GROUP_NONE)
{
}

bool botsharedutils::TraceFilterVisibleSimple::ShouldHitEntity(IHandleEntity* pHandleEntity, int contentsMask)
{
	if (trace::CTraceFilterSimple::ShouldHitEntity(pHandleEntity, contentsMask))
	{
		CBaseEntity* pEntity = trace::EntityFromEntityHandle(pHandleEntity);

		if (!pEntity)
		{
			return true; // static prop
		}

		return EntityBlocksLOS(pEntity);
	}

	return false;
}

bool botsharedutils::TraceFilterVisibleSimple::EntityBlocksLOS(CBaseEntity* entity)
{
	int entindex = UtilHelpers::IndexOfEntity(entity);

	if (entindex == 0) { return true; } // worldspawn, always blocks

	if (entindex >= 1 && entindex <= gpGlobals->maxClients)
	{
		return false; // player, don't block LOS
	}

	if (UtilHelpers::FClassnameIs(entity, "prop_phys*"))
	{
		return false; // assume physics props can be pushed and doesn't block LOS
	}

	if (UtilHelpers::FClassnameIs(entity, "obj_*"))
	{
		return false; // Buildings don't block LOS
	}

	return true;
}

botsharedutils::RandomDefendSpotCollector::RandomDefendSpotCollector(const Vector& spot, CBaseBot* bot) :
	m_vecOffset(0.0f, 0.0f, 36.0f)
{
	m_bot = bot;
	m_spot = spot;
	m_filter.SetPassEntity(bot->GetEntity());

	SetSearchElevators(false);
	SetTravelLimit(bot->GetSensorInterface()->GetMaxVisionRange());

	CNavArea* area = TheNavMesh->GetNavArea(spot);
	SetStartArea(area);
}

bool botsharedutils::RandomDefendSpotCollector::ShouldSearch(CNavArea* area)
{
	// Don't search into blocked areas for my team
	return !area->IsBlocked(m_bot->GetCurrentTeamIndex());
}

bool botsharedutils::RandomDefendSpotCollector::ShouldCollect(CNavArea* area)
{
	if (area == GetStartArea())
	{
		return false;
	}

	if (!m_bot->GetMovementInterface()->IsAreaTraversable(area))
	{
		return false;
	}

	trace_t tr;
	trace::line(m_spot, area->GetCenter() + m_vecOffset, MASK_VISIBLE, &m_filter, tr);

	if (tr.fraction < 1.0f)
	{
		return false; // area can't be seen from the position we're going to defend
	}

	return true;
}

botsharedutils::RandomSnipingSpotCollector::RandomSnipingSpotCollector(const Vector& spot, CBaseBot* bot, const float maxRange) :
	m_vecOffset1(0.0f, 0.0f, 0.0f), m_vecOffset2(0.0f, 0.0f, 0.0f), m_spot(spot)
{
	m_bot = bot;
	m_snipeArea = nullptr;
	m_filter.SetPassEntity(bot->GetEntity());

	float halfvisrange = (bot->GetSensorInterface()->GetMaxVisionRange() * 0.5f);
	m_minRange = std::clamp(halfvisrange, 256.0f, 512.0f);

	SetSearchElevators(false);
	SetTravelLimit(std::clamp(bot->GetSensorInterface()->GetMaxVisionRange(), 0.0f, maxRange));

	CNavArea* area = TheNavMesh->GetNavArea(spot);
	SetStartArea(area);
}

bool botsharedutils::RandomSnipingSpotCollector::ShouldSearch(CNavArea* area)
{
	return !area->IsBlocked(m_bot->GetCurrentTeamIndex());
}

bool botsharedutils::RandomSnipingSpotCollector::ShouldCollect(CNavArea* area)
{
	if (m_bot->GetRangeTo(area->GetCenter()) < m_minRange)
	{
		return false; // too close
	}

	if (!m_bot->GetMovementInterface()->IsAreaTraversable(area))
	{
		return false;
	}

	bool nolos = (BlockedLOS(area->GetCenter() + m_vecOffset1) && BlockedLOS(area->GetCenter() + m_vecOffset2));

	if (nolos) { return false; }

	return true;
}

void botsharedutils::RandomSnipingSpotCollector::OnDone()
{
	// Further filter the collected areas.
	
	constexpr float HIGH_GROUND_LIMIT = 450.0f;
	float lastZ = -999999.0f;
	float lastRange = 999999.0f;
	CNavArea* candidate = nullptr;

	for (CNavArea* area : GetCollectedAreas())
	{
		float z = std::abs((area->GetCenter().z - m_spot.z));
		float range = (area->GetCenter() - m_spot).Length();

		if (range < lastRange && z < HIGH_GROUND_LIMIT && z > lastZ)
		{
			lastZ = z;
			lastRange = range;
			candidate = area;
		}
	}

	m_snipeArea = candidate;
}

bool botsharedutils::RandomSnipingSpotCollector::BlockedLOS(Vector endPos)
{
	trace_t tr;
	trace::line(m_spot, endPos, MASK_VISIBLE, &m_filter, tr);
	return tr.fraction < 1.0f;
}

botsharedutils::FindCoverCollector::FindCoverCollector(const Vector& fromSpot, const float radius, const bool checkLOS, const bool checkCorners, const float maxSearchRange, CBaseBot* bot) :
	m_vecOffset(0.0f, 0.0f, 32.0f)
{
	m_coverFrom = fromSpot;
	m_minDistance = radius;
	m_checkLOS = checkLOS;
	m_checkCorners = checkCorners;
	m_bot = bot;
	m_myPos = bot->GetAbsOrigin();
	m_dir = (m_myPos - m_coverFrom);
	m_dir.NormalizeInPlace();
	m_cos = UtilHelpers::GetForwardViewCone(120.0f);
	SetStartArea(bot->GetLastKnownNavArea());
	SetTravelLimit(maxSearchRange);
	SetSearchElevators(false); // too slow for cover
}

botsharedutils::FindCoverCollector::FindCoverCollector(const Vector& fromSpot, const float radius, const bool checkLOS, const bool checkCorners, const float maxSearchRange, const Vector& origin) :
	m_vecOffset(0.0f, 0.0f, 32.0f)
{
	m_coverFrom = fromSpot;
	m_minDistance = radius;
	m_checkLOS = checkLOS;
	m_checkCorners = checkCorners;
	m_bot = nullptr;
	m_myPos = origin;
	m_dir = (m_myPos - m_coverFrom);
	m_dir.NormalizeInPlace();
	m_cos = UtilHelpers::GetForwardViewCone(120.0f);
	SetStartArea(TheNavMesh->GetNavArea(origin));
	SetTravelLimit(maxSearchRange);
	SetSearchElevators(false); // too slow for cover
}

bool botsharedutils::FindCoverCollector::ShouldSearch(CNavArea* area)
{
	if (m_bot)
	{
		return !area->IsBlocked(m_bot->GetCurrentTeamIndex());
	}

	return true;
}

bool botsharedutils::FindCoverCollector::ShouldCollect(CNavArea* area)
{
	if (m_bot)
	{
		if (!m_bot->GetMovementInterface()->IsAreaTraversable(area))
		{
			return false;
		}
	}

	float range = (m_coverFrom - area->GetCenter()).Length();

	if (range <= m_minDistance)
	{
		return false;
	}

	float range2 = (m_myPos - area->GetCenter()).Length();

	// area is closer to the cover from spot
	if (range < range2)
	{
		return false;
	}

	// skip areas that would make us move towards the threat!
	if (UtilHelpers::PointWithinViewAngle(m_myPos, area->GetCenter(), m_dir, m_cos))
	{
		return false;
	}
	
	if (m_checkLOS)
	{
		trace_t tr;

		trace::line(m_coverFrom, area->GetCenter() + m_vecOffset, MASK_VISIBLE, tr);

		if (!tr.DidHit())
		{
			return false; // has LOS to center
		}

		if (m_checkCorners)
		{
			for (int corner = 0; corner < static_cast<int>(NavCornerType::NUM_CORNERS); corner++)
			{
				trace::line(m_coverFrom, area->GetCorner(static_cast<NavCornerType>(corner)) + m_vecOffset, MASK_VISIBLE, tr);

				if (!tr.DidHit())
				{
					return false; // has LOS to corner
				}
			}
		}
	}

	return true;
}

float botsharedutils::weapons::GetMaxAttackRangeForCurrentlyHeldWeapon(CBaseBot* bot)
{
	const CBotWeapon* weapon = bot->GetInventoryInterface()->GetActiveBotWeapon();

	if (!weapon)
	{
		return -999999.0f;
	}

	float range = 0.0f;

	if (bot->GetControlInterface()->GetLastUsedAttackType() == IPlayerController::AttackType::ATTACK_NONE ||
		bot->GetControlInterface()->GetLastUsedAttackType() == IPlayerController::AttackType::ATTACK_PRIMARY)
	{
		range = weapon->GetWeaponInfo()->GetAttackInfo(WeaponInfo::AttackFunctionType::PRIMARY_ATTACK).GetMaxRange();
	}
	else
	{
		range = weapon->GetWeaponInfo()->GetAttackInfo(WeaponInfo::AttackFunctionType::SECONDARY_ATTACK).GetMaxRange();
	}

	return range;
}

Vector botsharedutils::aiming::GetAimPositionForPlayers(CBaseBot* bot, CBaseExtPlayer* player, IDecisionQuery::DesiredAimSpot desiredAim, const CBotWeapon* weapon, const char* headbone)
{
	Vector wepOffset = weapon->GetWeaponInfo()->GetHeadShotAimOffset();

	Vector theirPos = vec3_origin;

	switch (desiredAim)
	{
	case IDecisionQuery::AIMSPOT_ABSORIGIN:
		theirPos = player->GetAbsOrigin();
		break;
	case IDecisionQuery::AIMSPOT_CENTER:
		theirPos = player->WorldSpaceCenter();
		break;
	case IDecisionQuery::AIMSPOT_HEAD:
	{
		if (headbone)
		{
			player->GetHeadShotPosition(headbone, theirPos);
			theirPos + wepOffset;
		}
		else
		{
			theirPos = player->GetEyeOrigin() + wepOffset;
		}

		break;
	}
	case IDecisionQuery::AIMSPOT_OFFSET:
		theirPos = player->GetAbsOrigin() + bot->GetControlInterface()->GetCurrentDesiredAimOffset();
		break;
	case IDecisionQuery::AIMSPOT_BONE:
	{
		auto model = UtilHelpers::GetEntityModelPtr(player->GetEdict());

		if (model)
		{
			QAngle angle;
			Vector pos;

			if (UtilHelpers::GetBonePosition(player->GetEntity(), model.get(), bot->GetControlInterface()->GetCurrentDesiredAimBone().c_str(), pos, angle))
			{
				theirPos = pos;
			}
		}

		break;
	}
	default:
		theirPos = player->WorldSpaceCenter();
		break;
	}

	return theirPos;
}

Vector botsharedutils::aiming::AimAtPlayerWithHitScan(CBaseBot* bot, CBaseEntity* target, IDecisionQuery::DesiredAimSpot desiredAim, const CBotWeapon* weapon, const char* headbone)
{
	CBaseExtPlayer enemy{ UtilHelpers::BaseEntityToEdict(target) };
	return botsharedutils::aiming::GetAimPositionForPlayers(bot, &enemy, desiredAim, weapon, headbone);
}

Vector botsharedutils::aiming::AimAtPlayerWithProjectile(CBaseBot* bot, CBaseEntity* target, IDecisionQuery::DesiredAimSpot desiredAim, const CBotWeapon* weapon, const char* headbone, const bool checkLOS)
{
	CBaseExtPlayer enemy{ UtilHelpers::BaseEntityToEdict(target) };

	Vector wepOffset = weapon->GetWeaponInfo()->GetHeadShotAimOffset();
	Vector theirPos = botsharedutils::aiming::GetAimPositionForPlayers(bot, &enemy, desiredAim, weapon, headbone);

	WeaponInfo::AttackFunctionType type = WeaponInfo::AttackFunctionType::PRIMARY_ATTACK;

	if (bot->GetControlInterface()->GetLastUsedAttackType() == IPlayerInput::AttackType::ATTACK_SECONDARY)
	{
		type = WeaponInfo::AttackFunctionType::SECONDARY_ATTACK;
	}

	float range = (theirPos - bot->GetEyeOrigin()).Length();
	Vector predicted = pred::SimpleProjectileLead(theirPos, enemy.GetAbsVelocity(), weapon->GetWeaponInfo()->GetAttackInfo(type).GetProjectileSpeed(), range);

	if (checkLOS && !bot->IsLineOfFireClear(predicted))
	{
		// obstruction between the predicted position and the bot, just fire at the enemy's center
		return enemy.WorldSpaceCenter();
	}

	return predicted;
}

Vector botsharedutils::aiming::AimAtPlayerWithBallistic(CBaseBot* bot, CBaseEntity* target, IDecisionQuery::DesiredAimSpot desiredAim, const CBotWeapon* weapon, const char* headbone)
{
	CBaseExtPlayer enemy{ UtilHelpers::BaseEntityToEdict(target) };

	Vector wepOffset = weapon->GetWeaponInfo()->GetHeadShotAimOffset();
	Vector theirPos = botsharedutils::aiming::GetAimPositionForPlayers(bot, &enemy, desiredAim, weapon, headbone);

	WeaponInfo::AttackFunctionType type = WeaponInfo::AttackFunctionType::PRIMARY_ATTACK;

	if (bot->GetControlInterface()->GetLastUsedAttackType() == IPlayerInput::AttackType::ATTACK_SECONDARY)
	{
		type = WeaponInfo::AttackFunctionType::SECONDARY_ATTACK;
	}

	float range = (theirPos - bot->GetEyeOrigin()).Length();
	auto& attackinfo = weapon->GetWeaponInfo()->GetAttackInfo(type);
	Vector predicted = pred::SimpleProjectileLead(theirPos, enemy.GetAbsVelocity(), weapon->GetWeaponInfo()->GetAttackInfo(type).GetProjectileSpeed(), range);

	Vector myEyePos = bot->GetEyeOrigin();
	Vector enemyVel = enemy.GetAbsVelocity();
	float rangeTo = (myEyePos - theirPos).Length();
	const float projSpeed = attackinfo.GetProjectileSpeed();
	const float gravity = attackinfo.GetGravity();

	Vector aimPos = pred::SimpleProjectileLead(theirPos, enemyVel, projSpeed, rangeTo);
	rangeTo = (myEyePos - aimPos).Length();

	const float elevation_rate = RemapValClamped(rangeTo, attackinfo.GetBallisticElevationStartRange(), attackinfo.GetBallisticElevationEndRange(), attackinfo.GetBallisticElevationMinRate(), attackinfo.GetBallisticElevationMaxRate());
	float z = pred::GravityComp(rangeTo, gravity, elevation_rate);
	aimPos.z += z;

	return aimPos;
}

Vector botsharedutils::aiming::GetAimPositionForEntities(CBaseBot* bot, CBaseEntity* target, IDecisionQuery::DesiredAimSpot desiredAim, const CBotWeapon* weapon)
{
	Vector wepOffset = weapon->GetWeaponInfo()->GetHeadShotAimOffset();

	Vector theirPos = vec3_origin;

	switch (desiredAim)
	{
	case IDecisionQuery::AIMSPOT_ABSORIGIN:
		theirPos = UtilHelpers::getEntityOrigin(target);
		break;
	case IDecisionQuery::AIMSPOT_CENTER:
		theirPos = UtilHelpers::getWorldSpaceCenter(target);
		break;
	case IDecisionQuery::AIMSPOT_HEAD:
	{
		Vector* viewOffset = entprops->GetPointerToEntData<Vector>(target, Prop_Data, "m_vecViewOffset");

		if (viewOffset)
		{
			theirPos = UtilHelpers::getEntityOrigin(target);
			theirPos = theirPos + (*viewOffset);
		}
		else
		{
			ICollideable* collider = reinterpret_cast<IServerEntity*>(target)->GetCollideable();
			const Vector& maxs = collider->OBBMaxs();
			const Vector& mins = collider->OBBMins();
			float z = abs(maxs.z - mins.z);
			z *= 0.90;
			theirPos = UtilHelpers::getEntityOrigin(target);
			theirPos.z += z;
		}

		break;
	}
	case IDecisionQuery::AIMSPOT_OFFSET:
		theirPos = UtilHelpers::getEntityOrigin(target) + bot->GetControlInterface()->GetCurrentDesiredAimOffset();
		break;
	case IDecisionQuery::AIMSPOT_BONE:
	{
		edict_t* edict = UtilHelpers::BaseEntityToEdict(target);

		if (edict)
		{
			auto model = UtilHelpers::GetEntityModelPtr(edict);

			if (model)
			{
				QAngle angle;
				Vector pos;

				if (UtilHelpers::GetBonePosition(target, model.get(), bot->GetControlInterface()->GetCurrentDesiredAimBone().c_str(), pos, angle))
				{
					return pos;
				}
			}
		}

		// bone look up failed
		theirPos = UtilHelpers::getWorldSpaceCenter(target);
		break;
	}
	default:
		theirPos = UtilHelpers::getWorldSpaceCenter(target);
		break;
	}

	return theirPos;
}

Vector botsharedutils::aiming::AimAtEntityWithBallistic(CBaseBot* bot, CBaseEntity* target, IDecisionQuery::DesiredAimSpot desiredAim, const CBotWeapon* weapon)
{
	Vector wepOffset = weapon->GetWeaponInfo()->GetHeadShotAimOffset();
	Vector theirPos = botsharedutils::aiming::GetAimPositionForEntities(bot, target, desiredAim, weapon);

	WeaponInfo::AttackFunctionType type = WeaponInfo::AttackFunctionType::PRIMARY_ATTACK;

	if (bot->GetControlInterface()->GetLastUsedAttackType() == IPlayerInput::AttackType::ATTACK_SECONDARY)
	{
		type = WeaponInfo::AttackFunctionType::SECONDARY_ATTACK;
	}

	float range = (theirPos - bot->GetEyeOrigin()).Length();
	auto& attackinfo = weapon->GetWeaponInfo()->GetAttackInfo(type);
	Vector velocity{ 0.0f, 0.0f, 0.0f };
	entprops->GetEntPropVector(UtilHelpers::IndexOfEntity(target), Prop_Data, "m_vecVelocity", velocity);
	Vector predicted = pred::SimpleProjectileLead(theirPos, velocity, weapon->GetWeaponInfo()->GetAttackInfo(type).GetProjectileSpeed(), range);

	Vector myEyePos = bot->GetEyeOrigin();
	float rangeTo = (myEyePos - theirPos).Length();
	const float projSpeed = attackinfo.GetProjectileSpeed();
	const float gravity = attackinfo.GetGravity();

	Vector aimPos = pred::SimpleProjectileLead(theirPos, velocity, projSpeed, rangeTo);
	rangeTo = (myEyePos - aimPos).Length();

	const float elevation_rate = RemapValClamped(rangeTo, attackinfo.GetBallisticElevationStartRange(), attackinfo.GetBallisticElevationEndRange(), attackinfo.GetBallisticElevationMinRate(), attackinfo.GetBallisticElevationMaxRate());
	float z = pred::GravityComp(rangeTo, gravity, elevation_rate);
	aimPos.z += z;

	return aimPos;
}

void botsharedutils::aiming::SelectDesiredAimSpotForTarget(CBaseBot* bot, CBaseEntity* target)
{
	const Vector& maxs = reinterpret_cast<IServerEntity*>(target)->GetCollideable()->OBBMaxs();
	Vector center = UtilHelpers::getWorldSpaceCenter(target);
	const Vector& origin = UtilHelpers::getEntityOrigin(target);
	Vector head = origin + maxs;
	head.z -= 2.0f;

	const bool headisclear = bot->IsLineOfFireClear(head);

	if (bot->GetDifficultyProfile()->IsAllowedToHeadshot() && headisclear)
	{
		bot->GetControlInterface()->SetDesiredAimSpot(IDecisionQuery::DesiredAimSpot::AIMSPOT_HEAD);
	}
	else if (bot->IsLineOfFireClear(center))
	{
		bot->GetControlInterface()->SetDesiredAimSpot(IDecisionQuery::DesiredAimSpot::AIMSPOT_CENTER);
	}
	else if (bot->IsLineOfFireClear(origin))
	{
		bot->GetControlInterface()->SetDesiredAimSpot(IDecisionQuery::DesiredAimSpot::AIMSPOT_ABSORIGIN);
	}
	else if (headisclear) // this is a fallback for when the bot skips the head due to not being allowed to headshot but LOF is blocked for center and origin.
	{
		bot->GetControlInterface()->SetDesiredAimSpot(IDecisionQuery::DesiredAimSpot::AIMSPOT_CENTER);
	}
}

botsharedutils::RandomDestinationCollector::RandomDestinationCollector(CBaseBot* bot, const float travellimit) :
	INavAreaCollector<CNavArea>(bot->GetLastKnownNavArea(), travellimit)
{
	m_bot = bot;
}

botsharedutils::RandomDestinationCollector::RandomDestinationCollector(CNavArea* start, const float travellimit) :
	INavAreaCollector<CNavArea>(start, travellimit)
{
	m_bot = nullptr;
}

bool botsharedutils::RandomDestinationCollector::ShouldSearch(CNavArea* area)
{
	if (m_bot)
	{
		return m_bot->GetMovementInterface()->IsAreaTraversable(area);
	}

	return !area->IsBlocked(NAV_TEAM_ANY);
}

botsharedutils::IsReachableAreas::IsReachableAreas(CBaseBot* bot, const float limit, const bool searchLadders, const bool searchLinks, const bool searchElevators) :
	INavAreaCollector<CNavArea>(bot->GetLastKnownNavArea(), limit, searchLadders, searchLinks, searchElevators)
{
	m_bot = bot;
}

bool botsharedutils::IsReachableAreas::ShouldSearch(CNavArea* area)
{
	return !area->IsBlocked(m_bot->GetCurrentTeamIndex());
}

const bool botsharedutils::IsReachableAreas::IsReachable(CNavArea* area, float* cost) const
{
	auto node = GetNodeForArea(area);

	if (node)
	{
		if (cost)
		{
			*cost = node->GetTravelCostFromStart();
		}

		return true;
	}

	return false;
}

botsharedutils::SpreadDangerToNearbyAreas::SpreadDangerToNearbyAreas(CNavArea* start, int teamid, const float travellimit, const float danger, const float maxdanger) :
	INavAreaCollector<CNavArea>(start, travellimit)
{
	m_danger = danger;
	m_limit = maxdanger;
	m_teamid = teamid;
}

bool botsharedutils::SpreadDangerToNearbyAreas::ShouldSearch(CNavArea* area)
{
	return !area->IsBlocked(m_teamid);
}

bool botsharedutils::SpreadDangerToNearbyAreas::ShouldCollect(CNavArea* area)
{
	return true;
}

void botsharedutils::SpreadDangerToNearbyAreas::OnDone()
{
	for (CNavArea* area : GetCollectedAreas())
	{
		float danger = area->GetDanger(m_teamid) + m_danger;

		if (danger > m_limit)
		{
			danger = m_limit - area->GetDanger(m_teamid);

			if (danger <= 0.0f)
			{
				continue;
			}

			area->IncreaseDanger(m_teamid, danger);
			continue;
		}

		area->IncreaseDanger(m_teamid, m_danger);
	}
}
