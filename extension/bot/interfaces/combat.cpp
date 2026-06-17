#include NAVBOT_PCH_FILE
#include <string_view>
#include <extension.h>
#include <mods/modhelpers.h>
#include <navmesh/nav_mesh.h>
#include <bot/basebot.h>
#include <bot/bot_shared_utils.h>
#include "combat.h"

#ifdef EXT_VPROF_ENABLED
#include <tier0/vprof.h>
#endif // EXT_VPROF_ENABLED

ICombat::ICombat(CBaseBot* bot) :
	IBotInterface(bot)
{
	m_lastWeaponPtr = nullptr;
	m_lastThreatPtr = nullptr;
	m_shouldAim = true;
	m_shouldSelectWeapons = true;
	m_isScopedOrDeployed = false;
	m_wantsToScope = false;
	m_reAim = false;
	m_shouldReloadPostCombat = false;
	m_lastPlace = static_cast<unsigned int>(UNDEFINED_PLACE);
	m_combatData.Clear();
}

ICombat::~ICombat()
{
}

void ICombat::Reset()
{
	m_lastWeaponPtr = nullptr;
	m_lastThreatPtr = nullptr;
	m_shouldAim = true;
	m_shouldSelectWeapons = true;
	m_isScopedOrDeployed = false;
	m_wantsToScope = false;
	m_reAim = false;
	m_lastPlace = static_cast<unsigned int>(UNDEFINED_PLACE);
	m_dangerScanTimer.Invalidate();
	m_disableCombatTimer.Invalidate();
	m_dontFireTimer.Invalidate();
	m_toggleScopeTimer.Invalidate();
	m_useSpecialFuncTimer.Invalidate();
	m_selectWeaponTimer.Invalidate();
	m_secondaryAbilityTimer.Invalidate();
	m_scopeinDelayTimer.Invalidate();
	m_calloutTimer.Invalidate();
	m_reloadTimer.Invalidate();
	m_disableLookingAroundTimer.Invalidate();
	m_disableDodgeTimer.Invalidate();
	m_blockFiringTimer.Invalidate();
	m_attackTimer.Invalidate();
	m_clearAreasTimer.Invalidate();
	m_lookAroundTimer.Start(LOOK_AROUND_TIMER_BASE_MAX);
	m_combatData.Clear();
	m_lastDangerEntity.Term();
}

void ICombat::Update()
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("ICombat::Update", "NavBot");
#endif // EXT_VPROF_ENABLED

	if (!m_disableCombatTimer.IsElapsed()) { return; }

	const CBaseBot* bot = GetBot<const CBaseBot>();

	if (!bot->IsAlive()) { return; }

	const CKnownEntity* threat = bot->GetSensorInterface()->GetPrimaryKnownThreat(ISensor::ANY_THREATS);

	if (threat)
	{
		if (ShouldSelectWeapons() && m_selectWeaponTimer.IsElapsed())
		{
			// call this before GetActiveBotWeapon
			if (bot->GetInventoryInterface()->SelectBestWeaponForThreat(threat))
			{
				DisableFiringWeapons(0.2f, true); // just switched weapons, let go of buttons for a little
			}
		}

		const CBotWeapon* activeWeapon = bot->GetInventoryInterface()->GetActiveBotWeapon();

		if (!activeWeapon)
		{
			return;
		}

		bool wasNotVisible = false;

		if (!m_combatData.in_combat)
		{
			OnStartCombat(threat, activeWeapon);
		}
		else
		{
			// is in combat already
			if (!m_combatData.is_visible)
			{
				// flag as not visible before the update
				wasNotVisible = true;
			}
		}

		SetLastThreat(threat);
		UpdateCombatData(threat, activeWeapon);
		GetPostCombatChecksTimer().Start(POST_COMBAT_TIMER_DURATION);

		if (m_combatData.is_visible && wasNotVisible)
		{
			// threat was previously not visible and just became visible
			OnThreatBecameVisible(threat, activeWeapon);
		}

		if (!m_combatData.is_visible && !m_combatData.ten_seconds_passed && m_combatData.GetTimeSinceLostLOS() >= 10.0f)
		{
			m_combatData.ten_seconds_passed = true;
			OnTenSecondsSinceThreatVisible();
		}

		// if the threat is no longer visible and more than 5 seconds have passed, unscope.
		if (!m_combatData.is_visible && WantstoBeScoped() && GetTimeSinceLOSWasLost() >= 5.0f)
		{
			SetWantsToScopeState(false);
		}

		if (ShouldAim())
		{
			CombatAim(bot, threat, activeWeapon);

			if (!m_reAim && !m_combatData.is_visible)
			{
				// This makes the bot wait a single update tick before firing their weapons after acquiring LOS again
				m_reAim = true;
			}
		}

		FireWeaponAtEnemy(bot, threat, activeWeapon);
		DodgeEnemies(threat, activeWeapon);
		UseSecondaryAbilities(threat, activeWeapon);
	}
	else
	{
		m_combatData.in_combat = false;

		// no threat, unscope
		if (WantstoBeScoped())
		{
			SetWantsToScopeState(false);
		}

		if (CanLookAround() && m_lookAroundTimer.IsElapsed())
		{
			m_lookAroundTimer.StartRandom(LOOK_AROUND_TIMER_BASE_MIN, LOOK_AROUND_TIMER_BASE_MAX);

			UpdateLookingAround();
		}

		if (GetPostCombatChecksTimer().HasStarted() && GetPostCombatChecksTimer().IsElapsed())
		{
			GetPostCombatChecksTimer().Invalidate();
			OnPostCombat();
		}
	}

	UpdateScopeState();
	DangerScanUpdate();
	UpdateMarkAreasAsCleared(threat);
}

void ICombat::Frame()
{
}

void ICombat::DisableFiringWeapons(const float time, const bool letGoOfButtons)
{
	m_dontFireTimer.Start(time);

	if (letGoOfButtons)
	{
		GetBot<CBaseBot>()->GetControlInterface()->ReleaseAllAttackButtons();
	}
}

void ICombat::FireWeaponAt(CBaseEntity* entity, const bool doAim)
{
	CBaseBot* bot = GetBot<CBaseBot>();
	const CBotWeapon* weapon = bot->GetInventoryInterface()->GetActiveBotWeapon();

	if (!weapon)
		return;

	CKnownEntity* threat = bot->GetSensorInterface()->AddKnownEntity(entity);
	
	UpdateCombatData(threat, weapon);

	if (doAim)
	{
		CombatAim(bot, threat, weapon);
	}

	FireWeaponAtEnemy(bot, threat, weapon);
}

bool ICombat::ScopeInOrDeployWeapon()
{
	CBaseBot* bot = GetBot<CBaseBot>();
	const CBotWeapon* weapon = bot->GetInventoryInterface()->GetActiveBotWeapon();

	if (!weapon) { return false; }

	m_isScopedOrDeployed = weapon->IsDeployedOrScoped(bot);

	if (!m_isScopedOrDeployed)
	{
		bot->GetControlInterface()->PressSecondaryAttackButton();

		const float time = weapon->GetWeaponInfo()->GetScopeInAttackDelay();

		if (time > 0.0f)
		{
			GetScopeInDelayTimer().Start(time);
		}
	}

	return m_isScopedOrDeployed;
}

void ICombat::DoPrimaryAttack()
{
	CBaseBot* bot = GetBot<CBaseBot>();
	const CBotWeapon* weapon = bot->GetInventoryInterface()->GetActiveBotWeapon();

	if (!weapon) { return; }

	SetLastUsedWeapon(weapon->GetEntity());
	const WeaponInfo* info = weapon->GetWeaponInfo();
	IntervalTimer& timer = GetAttackTimer();

	if (timer.HasStarted() && timer.IsLessThen(info->GetAttackInfo(botweapons::AttackType::PRIMARY).GetDelayBetweenAttacks()))
	{
		return;
	}

	if (!weapon->IsLoaded())
	{
		bot->GetControlInterface()->PressReloadButton();
		return;
	}

	timer.Start();
	bot->GetControlInterface()->PressAttackButton(info->GetAttackInfo(botweapons::AttackType::PRIMARY).GetHoldButtonTime());
}

const bool ICombat::CanLookAround() const
{
	// makes low difficulty bots easy to approach from behind
	if (GetBot<CBaseBot>()->GetDifficultyProfile()->GetGameAwareness() <= LOOK_AROUND_MIN_SKILL)
	{
		return false;
	}

	return m_disableLookingAroundTimer.IsElapsed();
}

bool ICombat::DangerScanParseGamedata()
{
	SourceMod::IGameConfig* gcfg = extension->GetExtensionGameData();

	const char* value = gcfg->GetKeyValue("ICombat_AllowDangerScan");

	// outdated gamedata file
	if (!value)
	{
		return false;
	}

	ICombat::s_allowDangerScan = UtilHelpers::StringToBoolean(value);

	value = gcfg->GetKeyValue("ICombat_DoTeamChecks");

	// outdated gamedata file
	if (!value)
	{
		return false;
	}

	ICombat::s_dangerCheckTeam = UtilHelpers::StringToBoolean(value);

	value = gcfg->GetKeyValue("ICombat_DangerEnts");

	if (value && std::strlen(value) > 3) /* the danger entity list is optional */
	{
		std::string strvalue(value);
		std::stringstream stream(strvalue);
		std::string token;

		while (std::getline(stream, token, ','))
		{
			ICombat::s_dangerEnts.emplace(token);
		}
	}

#ifdef EXT_DEBUG
	META_CONPRINTF("[NavBot] Danger Scan enabled: %s. Number of Ents: %zu \n", UtilHelpers::textformat::FormatBool(ICombat::s_allowDangerScan), ICombat::s_dangerEnts.size());
#endif // EXT_DEBUG

	return true;
}

void ICombat::OnLastUsedWeaponChanged(CBaseEntity* newWeapon)
{
	m_attackTimer.Invalidate();
	m_isScopedOrDeployed = false;
	m_toggleScopeTimer.Invalidate();
	m_useSpecialFuncTimer.Invalidate();
	m_scopeinDelayTimer.Invalidate();
	m_reloadTimer.Invalidate();
	m_blockFiringTimer.Invalidate();
}

bool ICombat::CanFireWeaponAtEnemy(const CBaseBot* bot, const CKnownEntity* threat, const CBotWeapon* activeWeapon) const
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("ICombat::CanFireWeaponAtEnemy", "NavBot");
#endif // EXT_VPROF_ENABLED

	if (!m_dontFireTimer.IsElapsed())
		return false;

	if (!m_scopeinDelayTimer.IsElapsed())
		return false;

	if (!m_reloadTimer.IsElapsed())
		return false;

	if (bot->GetMovementInterface()->NeedsWeaponControl())
		return false;

	if (bot->GetBehaviorInterface()->ShouldAttack(GetBot<CBaseBot>(), threat) == ANSWER_NO)
		return false;

	if (!activeWeapon->GetWeaponInfo()->IsCombatWeapon())
		return false;

	if (bot->IsUnderWater() && !activeWeapon->GetWeaponInfo()->CanBeFiredUnderwater())
		return false;

	return true;
}

void ICombat::FireWeaponAtEnemy(const CBaseBot* bot, const CKnownEntity* threat, const CBotWeapon* activeWeapon, const bool bypassLOS)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("ICombat::FireWeaponAtEnemy", "NavBot");
#endif // EXT_VPROF_ENABLED

	if (!CanFireWeaponAtEnemy(bot, threat, activeWeapon)) { return; }

	// Update last used weapon
	SetLastUsedWeapon(activeWeapon->GetEntity());

	const CombatData& data = GetCachedCombatData();
	IPlayerController* input = bot->GetControlInterface();
	const WeaponInfo* info = activeWeapon->GetWeaponInfo();
	const bool canspam = (data.can_fire && info->CanBeSpammed() && GetTimeSinceLOSWasLost() <= info->GetSpamTime());
	auto& primInfo = info->GetAttackInfo(botweapons::AttackType::PRIMARY);

	// for melee weapons, see if the bot needs to crouch to hit something below then
	if (data.can_fire && primInfo.IsMelee() && data.enemy_range <= (primInfo.GetMaxRange() * 1.15f))
	{
		if (data.enemy_center.z < bot->GetAbsOrigin().z)
		{
			// enemy is below me
			input->PressCrouchButton(0.250f);
		}
	}

	if ((data.can_fire && data.is_visible) || canspam || bypassLOS) // visible and has clear line of fire
	{
		if (HandleWeapon(bot, activeWeapon))
		{
			// aiming at the current enemy
			if (input->GetAimAtTarget() == threat->GetEntity() && input->IsAimOnTarget())
			{
				OpportunisticallyUseWeaponSpecialFunction(bot, threat, activeWeapon);
				GetAttackTimer().Start(); // just attacked
				SetShouldReloadPostCombat(true); // remember to reload my weapon
				GetBlockFiringTimer().Start(info->GetAttackInfo(botweapons::GetValidAttackType(GetSelectedAttackType())).GetBlockAttackTime());

				if (data.can_use_primary)
				{
					input->PressAttackButton(info->GetAttackInfo(botweapons::AttackType::PRIMARY).GetHoldButtonTime());
				}
				else
				{
					input->PressSecondaryAttackButton(info->GetAttackInfo(botweapons::AttackType::SECONDARY).GetHoldButtonTime());
				}
			}
		}
		else // HandleWeapon returned false
		{
			OnHandleWeaponFailed(bot, threat, activeWeapon);
		}
	}
	else
	{
		if (!data.can_fire && data.is_visible)
		{
			if (bot->IsDebugging(BOTDEBUG_COMBAT))
			{
				bot->DebugPrintToConsole(255, 213, 128, "%s THREAT VISIBLE BUT LINE OF FIRE IS OBSTRUCTED! \n", bot->GetDebugIdentifier());
			}
		}
	}
}

bool ICombat::HandleWeapon(const CBaseBot* bot, const CBotWeapon* activeWeapon)
{
	IntervalTimer& timer = GetAttackTimer();
	const WeaponInfo* info = activeWeapon->GetWeaponInfo();
	const CombatData& data = GetCachedCombatData();
	IPlayerController* input = bot->GetControlInterface();
	const WeaponAttackFunctionInfo& attackfunc = info->GetAttackInfo(botweapons::GetValidAttackType(GetSelectedAttackType()));

	if (!data.in_range)
	{
		return false; // out side firing range
	}
	
	// check block attack timer
	if (!GetBlockFiringTimer().IsElapsed())
	{
		return false;
	}

	// check attack interval
	if (timer.HasStarted())
	{
		// attack uses distance mapped delays
		if (attackfunc.UsesDistanceMappedAttackDelays())
		{
			if (timer.IsLessThen(attackfunc.GetDistanceMappedAttackDelay(data.range_to_pos)))
			{
				return false;
			}
		}
		else // standard attack delay
		{
			if (timer.IsLessThen(attackfunc.GetDelayBetweenAttacks()))
			{
				return false;
			}
		}
	}

	// no ammo in clip
	if (!activeWeapon->IsLoaded())
	{
		return false;
	}

	SetScopedOrDeployedStatus(activeWeapon->IsDeployedOrScoped(bot));

	if (info->NeedsToBeDeployedToFire() && info->IsAllowedToScopeIn(data.range_to_pos))
	{
		SetWantsToScopeState(true);

		if (!IsScopedInOrDeployed())
		{
			return false;
		}
	}
	else
	{
		SetWantsToScopeState(false);
	}

	if (m_reAim)
	{
		// small reaim delay
		m_reAim = false;
		return false;
	}

	return true;
}

void ICombat::OnHandleWeaponFailed(const CBaseBot* bot, const CKnownEntity* threat, const CBotWeapon* activeWeapon)
{
	if (!activeWeapon->IsLoaded() && activeWeapon->HasAmmo(bot))
	{
		ReloadCurrentWeapon(bot, activeWeapon);
	}
}

void ICombat::ReloadCurrentWeapon(const CBaseBot* bot, const CBotWeapon* activeWeapon)
{
	IPlayerController* input = bot->GetControlInterface();
	input->ReleaseAllAttackButtons();
	input->PressReloadButton();
	GetReloadTimer().Start(1.0f); // to-do: per weapon settings for this?
}

void ICombat::OpportunisticallyUseWeaponSpecialFunction(const CBaseBot* bot, const CKnownEntity* threat, const CBotWeapon* weapon)
{
	CountdownTimer& timer = GetSpecialWeaponFunctionTimer();

	if (timer.IsElapsed())
	{
		const WeaponInfo::SpecialFunction& func = weapon->GetWeaponInfo()->GetSpecialFunction();
		const CombatData& data = GetCachedCombatData();
		IPlayerController* input = bot->GetControlInterface();

		if (weapon->CanUseSpecialFunction(bot, data.enemy_range))
		{
			timer.Start(func.delay_between_uses);

			if (bot->IsDebugging(BOTDEBUG_COMBAT))
			{
				bot->DebugPrintToConsole(255, 255, 0, "%s COMBAT: USING WEAPON SPECIAL FUNCTION %s! DELAY: %g\n", 
					bot->GetDebugIdentifier(), weapon->GetDebugIdentifier(), func.delay_between_uses);
			}

			input->PressButtonByID(func.button_to_press, func.hold_button_time);
		}
	}
}

bool ICombat::IsAbleToDodgeEnemies(const CKnownEntity* threat, const CBotWeapon* activeWeapon)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("ICombat::IsAbleToDodgeEnemies", "NavBot");
#endif // EXT_VPROF_ENABLED

	if (!GetDisableDodgeTimer().IsElapsed())
	{
		return false;
	}

	CBaseBot* bot = GetBot<CBaseBot>();

	// Low skill bot not allowed to dodge
	if (!bot->GetDifficultyProfile()->IsAllowedToDodge())
	{
		return false;
	}

	// Precise movement required for this area, don't dodge
	const CNavArea* area = bot->GetLastKnownNavArea();
	if (area && area->HasAttributes(static_cast<int>(NavAttributeType::NAV_MESH_PRECISE)))
	{
		return false;
	}

	const CombatData& cd = GetCachedCombatData();

	// Don't dodge if my line of fire is obstructed
	if (!cd.can_fire)
	{
		return false;
	}

	// Avoid messing up advanced movements
	if (bot->GetMovementInterface()->IsControllingMovements())
	{
		return false;
	}

	// Only dodge visible enemies
	if (!threat->IsVisibleNow())
	{
		return false;
	}

	// Only dodge enemy players
	if (!threat->IsPlayer())
	{
		return false;
	}

	Vector pos = bot->WorldSpaceCenter();

	// Don't dodge if the enemy player is not looking at my direction
	if (!threat->GetPlayerInstance()->IsLookingTowards(pos, 0.5f))
	{
		return false;
	}

	// Don't waste time dodging if I am in a hurry.
	if (bot->GetBehaviorInterface()->ShouldHurry(bot) == ANSWER_YES)
	{
		return false;
	}

	// Don't dodge if I'm not using a combat weapon
	if (!activeWeapon->GetWeaponInfo()->IsCombatWeapon() || !activeWeapon->GetWeaponInfo()->IsAllowedToDodge())
	{
		return false;
	}

	const WeaponAttackFunctionInfo& primaryAttackInfo = activeWeapon->GetWeaponInfo()->GetAttackInfo(botweapons::AttackType::PRIMARY);

	// Melee weapons: Don't dodge enemies if I'm close to the enemy
	if (primaryAttackInfo.IsMelee())
	{
		if (GetCachedCombatData().enemy_range <= primaryAttackInfo.GetMaxRange() * 2.0f)
		{
			return false;
		}
	}

	return true;
}

void ICombat::DodgeEnemies(const CKnownEntity* threat, const CBotWeapon* activeWeapon)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("ICombat::DodgeEnemies", "NavBot");
#endif // EXT_VPROF_ENABLED

	CBaseBot* bot = GetBot<CBaseBot>();

	if (!IsAbleToDodgeEnemies(threat, activeWeapon))
	{
		return;
	}

	Vector forward;
	Vector origin = bot->GetAbsOrigin();
	bot->EyeVectors(&forward);
	Vector left(-forward.y, forward.x, 0.0f);
	left.NormalizeInPlace();
	IMovement* mover = bot->GetMovementInterface();
	const float sideStepSize = mover->GetHullWidth();

	int rng = CBaseBot::s_botrng.GetRandomInt<int>(1, 100);

	if (rng < 33)
	{
		if (!mover->HasPotentialGap(origin, origin + sideStepSize * left))
		{
			bot->GetControlInterface()->PressMoveLeftButton();
		}
	}
	else if (rng > 66)
	{
		if (!mover->HasPotentialGap(origin, origin - sideStepSize * left))
		{
			bot->GetControlInterface()->PressMoveRightButton();
		}
	}

	if (CanJumpOrCrouchToDodge())
	{
		rng = CBaseBot::s_botrng.GetRandomInt<int>(1, 100);

		if (rng < 11)
		{
			bot->GetControlInterface()->PressCrouchButton(0.25f);
		}
		else if (rng > 86)
		{
			bot->GetControlInterface()->PressJumpButton();
		}
	}
}

void ICombat::OnStartCombat(const CKnownEntity* threat, const CBotWeapon* activeWeapon)
{
	const float time = activeWeapon->GetWeaponInfo()->GetInitialAttackDelay();

	if (time > 0.0f)
	{
		DisableFiringWeapons(time, true);
	}

	m_reAim = true;
	SetLastReportedPlace(static_cast<unsigned int>(UNDEFINED_PLACE));

	TryToCalloutThreat(threat);

	CBaseBot* bot = GetBot<CBaseBot>();

	if (bot->IsDebugging(BOTDEBUG_COMBAT))
	{
		bot->DebugPrintToConsole(255, 255, 0, "%s COMBAT: STARTING COMBAT AGAINST THREAT %s! CURRENT WEAPON: %s\n",
			bot->GetDebugIdentifier(), threat->GetDebugIdentifier(), activeWeapon->GetDebugIdentifier());
	}
}

void ICombat::OnThreatBecameVisible(const CKnownEntity* threat, const CBotWeapon* activeWeapon)
{
	CBaseBot* bot = GetBot<CBaseBot>();

	TryToCalloutThreat(threat);

	if (bot->IsDebugging(BOTDEBUG_COMBAT))
	{
		bot->DebugPrintToConsole(255, 255, 0, "%s COMBAT: ON THREAT %s BECAME VISIBLE! CURRENT WEAPON: %s\n",
			bot->GetDebugIdentifier(), threat->GetDebugIdentifier(), activeWeapon->GetDebugIdentifier());
	}
}

void ICombat::OnSelectedAttackTypeChanged(const CKnownEntity* threat, const CBotWeapon* activeWeapon, botweapons::AttackType oldtype)
{
	// this timer is now per attack
	GetAttackTimer().Invalidate();
}

bool ICombat::CanUseSecondaryAbilitities() const
{
	if (GetBot<CBaseBot>()->GetDifficultyProfile()->GetGameAwareness() <= 5)
	{
		return false;
	}

	if (!m_secondaryAbilityTimer.IsElapsed())
	{
		return false;
	}

	return true;
}

bool ICombat::CanCalloutThreat(const CKnownEntity* threat) const
{
	// Don't spam callouts
	if (!m_calloutTimer.IsElapsed())
	{
		return false;
	}

	CBaseBot* bot = GetBot<CBaseBot>();

	// This one doesn't like working with their team
	if (bot->GetDifficultyProfile()->GetTeamwork() < 25)
	{
		return false;
	}

	const CNavArea* area = threat->GetLastKnownArea();

	// Unknown last area
	if (!area)
	{
		return false;
	}

	// Area isn't named
	if (!area->HasPlaceName())
	{
		return false;
	}

	// Don't callout unless the place name changes
	if (static_cast<Place>(GetLastReportedPlace()) == area->GetPlace())
	{
		return false;
	}

	return true;
}

void ICombat::TryToCalloutThreat(const CKnownEntity* threat)
{
	if (CanCalloutThreat(threat))
	{
		SendCalloutOfTreat(threat);
	}
}

void ICombat::SendCalloutOfTreat(const CKnownEntity* threat)
{
	m_calloutTimer.Start(CALLOUT_COOLDOWN);
	// CanCalloutThreat already validates the current area (not NULL and has a name assigned to it)
	const CNavArea* area = threat->GetLastKnownArea();
	auto placename = TheNavMesh->GetPlaceDisplayName(area->GetPlace());

	if (placename.has_value())
	{
		SetLastReportedPlace(static_cast<unsigned int>(area->GetPlace()));
		CBaseBot* bot = GetBot<CBaseBot>();
		char message[256];
		const char* name = GetEnemyName(threat);

		ke::SafeSprintf(message, sizeof(message), "Enemy %s spotted at %s!", name, placename->c_str());
		bot->SendTeamChatMessage(message);

		if (bot->IsDebugging(BOTDEBUG_COMBAT))
		{
			bot->DebugPrintToConsole(255, 255, 0, "%s COMBAT: SENDING CALLOUT OF THREAT %s! NAV AREA #%u (%s)\n",
				bot->GetDebugIdentifier(), threat->GetDebugIdentifier(), area->GetID(), placename->c_str());
		}
	}
}

const char* ICombat::GetEnemyName(const CKnownEntity* enemy) const
{
	// classname by default, convenient because the player's entity classname is "player", so it's becomes "Enemy player ..."
	return enemy->GetEntityClassname().c_str();
}

void ICombat::UpdateLookingAround()
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("ICombat::UpdateLookingAround", "NavBot");
#endif // EXT_VPROF_ENABLED

	CBaseBot* me = GetBot<CBaseBot>();
	CNavArea* area = me->GetLastKnownNavArea();

	if (!area) { return; }

	combatutils::GetRandomNeighorAreaOutsideFOVFunctor functor{ me };
	area->ForEachAdjacentAndIncomingArea(functor);

	CNavArea* lookarea = functor.GetRandomArea();

	if (lookarea)
	{
		Vector aim = lookarea->GetCenter();
		aim.z += navgenparams->half_human_height;
		me->GetControlInterface()->AimAt(aim, IPlayerController::LookPriority::LOOK_INTERESTING, LOOK_AROUND_BASE_DURATION, "Looking around randomly.");
	}
}

void ICombat::OnPostCombat()
{
	CBaseBot* bot = GetBot<CBaseBot>();
	const bool isDebugging = bot->IsDebugging(BOTDEBUG_COMBAT);

	if (isDebugging)
	{
		bot->DebugPrintToConsole(255, 255, 0, "%s OnPostCombat\n", bot->GetDebugIdentifier());
	}

	if (GetShouldReloadPostCombat())
	{
		const CBotWeapon* activeWeapon = bot->GetInventoryInterface()->GetActiveBotWeapon();

		if (activeWeapon && activeWeapon->CanBeReloaded(bot))
		{
			if (isDebugging)
			{
				bot->DebugPrintToConsole(0, 255, 255, "%s RELOADING WEAPON POST COMBAT!\n", bot->GetDebugIdentifier());
			}

			bot->GetControlInterface()->PressReloadButton();
		}

		SetShouldReloadPostCombat(false);
	}
}

bool ICombat::CanScanForDanger() const
{
	if (!ICombat::s_allowDangerScan) { return false; }

	if (!GetBot<CBaseBot>()->GetDifficultyProfile()->IsAllowedToScanForDanger()) { return false; }

	if (!m_dangerScanTimer.IsElapsed()) { return false; }

	return true;
}

bool ICombat::IsEntityADangerSource(CBaseEntity* entity) const
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("ICombat::IsEntityADangerSource", "NavBot");
#endif // EXT_VPROF_ENABLED

	const char* classname = gamehelpers->GetEntityClassname(entity);

	// not on the list, not a danger
	if (ICombat::s_dangerEnts.find(classname) == ICombat::s_dangerEnts.cend())
	{
		return false;
	}

	if (ICombat::IsDangerScanTeamCheckEnabled())
	{
		// ignore entities from the same team
		if (GetBot<CBaseBot>()->GetCurrentTeamIndex() == modhelpers->GetEntityTeamNumber(entity))
		{
			return false;
		}
	}

	return true;
}

bool ICombat::IsAwareOfDangerEntity(CBaseEntity* entity) const
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("ICombat::IsAwareOfDangerEntity", "NavBot");
#endif // EXT_VPROF_ENABLED

	CBaseBot* bot = GetBot<CBaseBot>();
	ISensor* sensor = bot->GetSensorInterface();
	Vector center = UtilHelpers::getWorldSpaceCenter(entity);

	// something is obstructing my view of this entity.
	if (!sensor->IsLineOfSightClear(center))
	{
		return false;
	}

	// entity is not in my field of view (very high skill bots ignores this check).
	if (bot->GetDifficultyProfile()->GetGameAwareness() < ICombat::DANGER_SCAN_IGNORE_FOV_SKILL &&
		!sensor->IsInFieldOfView(center))
	{
		return false;
	}

	return true;
}

CBaseEntity* ICombat::SelectMostDangerousEntity(CBaseEntity* first, CBaseEntity* second) const
{
	const Vector origin = GetBot<CBaseBot>()->GetEyeOrigin();
	
	if ((origin - UtilHelpers::getWorldSpaceCenter(first)).LengthSqr() <= (origin - UtilHelpers::getWorldSpaceCenter(second)).LengthSqr())
	{
		return first;
	}

	return second;
}

void ICombat::OnTenSecondsSinceThreatVisible()
{
	CBaseBot* bot = GetBot<CBaseBot>();
	const bool isDebugging = bot->IsDebugging(BOTDEBUG_COMBAT);

	if (isDebugging)
	{
		bot->DebugPrintToConsole(255, 255, 0, "%s OnTenSecondsSinceThreatVisible\n", bot->GetDebugIdentifier());
	}

	// also do a reload check here since it may take a long time to enter post combat
	if (GetShouldReloadPostCombat())
	{
		const CBotWeapon* activeWeapon = bot->GetInventoryInterface()->GetActiveBotWeapon();

		if (activeWeapon && activeWeapon->CanBeReloaded(bot))
		{
			if (isDebugging)
			{
				bot->DebugPrintToConsole(0, 255, 255, "%s RELOADING WEAPON (NO LOS TO THREAT)!\n", bot->GetDebugIdentifier());
			}

			bot->GetControlInterface()->PressReloadButton();
		}

		SetShouldReloadPostCombat(false);
	}
}

IDecisionQuery::DesiredAimSpot ICombat::SelectClearAimSpot(const bool allowheadshots) const
{
	const CombatData& data = GetCachedCombatData();

	if (allowheadshots)
	{
		return IDecisionQuery::DesiredAimSpot::AIMSPOT_HEAD;
	}

	if (data.is_center_clear)
	{
		return IDecisionQuery::DesiredAimSpot::AIMSPOT_CENTER;
	}

	if (data.is_origin_clear)
	{
		return IDecisionQuery::DesiredAimSpot::AIMSPOT_ABSORIGIN;
	}

	// Not allowed to headshot but the head is the only visible body part
	if (data.is_head_clear)
	{
		return IDecisionQuery::DesiredAimSpot::AIMSPOT_HEAD;
	}

	return IDecisionQuery::DesiredAimSpot::AIMSPOT_CENTER;
}

void ICombat::CombatAim(const CBaseBot* bot, const CKnownEntity* threat, const CBotWeapon* activeWeapon)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("ICombat::CombatAim", "NavBot");
#endif // EXT_VPROF_ENABLED

	using namespace std::literals::string_view_literals;

	const CombatData& data = GetCachedCombatData();
	const DifficultyProfile* profile = bot->GetDifficultyProfile();
	IPlayerController* input = bot->GetControlInterface();
	const WeaponInfo* info = activeWeapon->GetWeaponInfo();
	ISensor* sensor = bot->GetSensorInterface();
	constexpr auto visible_priority = IPlayerController::LookPriority::LOOK_COMBAT;
	constexpr auto notvisible_priority = IPlayerController::LookPriority::LOOK_SEARCH;
	constexpr auto aimat_duration = 1.0f;
	constexpr auto combat_aim_reason = "Looking at visible threat!"sv;
	constexpr auto search_aim_reason = "Searching for threat!"sv;

	if (data.is_visible)
	{
		const bool canHeadshot = (profile->IsAllowedToHeadshot() && info->CanHeadShot() && data.is_head_clear && data.in_headshot_range);
		IDecisionQuery::DesiredAimSpot aimSpot = SelectClearAimSpot(canHeadshot);
		
		if (info->HasAimSpotPreference())
		{
			switch (info->GetPreferredAimSpot())
			{
			case IDecisionQuery::DesiredAimSpot::AIMSPOT_HEAD:
			{
				if (canHeadshot)
				{
					aimSpot = IDecisionQuery::DesiredAimSpot::AIMSPOT_HEAD;
				}

				break;
			}
			case IDecisionQuery::DesiredAimSpot::AIMSPOT_CENTER:
			{
				if (data.is_center_clear)
				{
					aimSpot = IDecisionQuery::DesiredAimSpot::AIMSPOT_CENTER;
				}

				break;
			}
			case IDecisionQuery::DesiredAimSpot::AIMSPOT_ABSORIGIN:
			{
				if (data.is_origin_clear)
				{
					aimSpot = IDecisionQuery::DesiredAimSpot::AIMSPOT_ABSORIGIN;
				}

				break;
			}
			default:
				break;
			}
		}

		input->SetDesiredAimSpot(aimSpot);
		input->AimAt(threat->GetEntity(), visible_priority, aimat_duration, combat_aim_reason.data());
	}
	else
	{
		// Simulate the bot having knowledge of hiding spots by aiming at where the enemy is if there is a clear line of sight bypassing FOV
		if (profile->GetGameAwareness() >= 80)
		{
			if (sensor->IsLineOfSightClear(threat->GetEntity()))
			{
				input->SetDesiredAimSpot(IDecisionQuery::DesiredAimSpot::AIMSPOT_CENTER);
				input->AimAt(data.enemy_center, notvisible_priority, aimat_duration, search_aim_reason.data());
			}
		}
	}
}

void ICombat::DangerScanUpdate()
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("ICombat::DangerScanUpdate", "NavBot");
#endif // EXT_VPROF_ENABLED

	if (!CanScanForDanger()) { return; }

	CBaseBot* bot = GetBot<CBaseBot>();
	GetDangerScanTimer().Start(bot->GetDifficultyProfile()->GetDangerScanFrequency());
	UtilHelpers::CEntityEnumerator collector;
	const float size = bot->GetDifficultyProfile()->GetDangerScanSize();
	Vector vSize{ size, size, bot->GetMovementInterface()->GetStandingHullHeight() * 3.0f };
	Vector origin = bot->GetAbsOrigin();
	Vector forward;
	bot->EyeVectors(&forward);
	// shift the origin to in-front of the bot
	origin = (origin + (forward * (bot->GetMovementInterface()->GetHullWidth() * 2.0f)));
	UtilHelpers::EntitiesInBox(origin - vSize, origin + vSize, collector);
	std::vector<CBaseEntity*> dangerEnts;

	auto func = [this, &dangerEnts](CBaseEntity* entity) {
		if (this->IsEntityADangerSource(entity) && this->IsAwareOfDangerEntity(entity))
		{
			dangerEnts.push_back(entity);
		}

		return true;
	};

	collector.ForEach(func);

	if (dangerEnts.empty()) { return; }

	CBaseEntity* first = nullptr;

	for (CBaseEntity* second : dangerEnts)
	{
		// assign first entity
		if (!first)
		{
			first = second;
			continue;
		}

		first = SelectMostDangerousEntity(first, second);

		// if selection returns NULL, stop the loop and set the last to NULL
		if (!first)
		{
			SetMostDangerousEntity(nullptr);
			return;
		}
	}

	EXT_ASSERT(first != nullptr, "DangerScanUpdate \"first\" entity is NULL!");

	CBaseEntity* last = GetMostDangerousEntity();

	// still the same entity since the last update
	if (last != nullptr && last == first) { return; }

	if (bot->IsDebugging(BOTDEBUG_COMBAT))
	{
		bot->DebugPrintToConsole(255, 255, 0, "%s DANGER SCAN FOUND NEW DANGEROUS ENTITY %s \n", bot->GetDebugIdentifier(), UtilHelpers::textformat::FormatEntity(first));
		NDebugOverlay::EntityBounds(first, 255, 0, 0, 127, 0.5f);
	}

	SetMostDangerousEntity(first);
	// dispatch event so behavior can react
	bot->OnDangerousEntityChanged(first, last);
}

void ICombat::UpdateScopeState()
{
	if (GetToggleScopeTimer().IsElapsed())
	{
		GetToggleScopeTimer().Start(TOGGLE_SCOPE_COOLDOWN_TIME);

		CBaseBot* bot = GetBot<CBaseBot>();
		const CBotWeapon* activeWeapon = bot->GetInventoryInterface()->GetActiveBotWeapon();

		if (activeWeapon)
		{
			SetScopedOrDeployedStatus(activeWeapon->IsDeployedOrScoped(bot));
			const bool wantstoscope = WantstoBeScoped();
			const bool isscoped = IsScopedInOrDeployed();

			// if the scoped state is different from what the bot wants
			if (wantstoscope != isscoped)
			{
				if (bot->IsDebugging(BOTDEBUG_COMBAT))
				{
					bot->DebugPrintToConsole(255, 255, 0, "%s COMBAT: TOGGLING SCOPE MODE! WEAPON %s! WANTS TO SCOPE: %i IS SCOPED: %i \n", 
						bot->GetDebugIdentifier(), activeWeapon->GetDebugIdentifier(), static_cast<int>(wantstoscope), static_cast<int>(isscoped));
				}

				// press the secondary attack button to change states
				// So far every game supported by navbot uses the secondary button
				bot->GetControlInterface()->PressSecondaryAttackButton();

				// If the bot wants to be scoped, apply the attack timer
				if (wantstoscope)
				{
					const float time = activeWeapon->GetWeaponInfo()->GetScopeInAttackDelay();

					if (time > 0.0f)
					{
						GetScopeInDelayTimer().Start(time);
					}
				}
			}
		}
	}
}

void ICombat::UpdateMarkAreasAsCleared(const CKnownEntity* threat)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("ICombat::UpdateMarkAreasAsCleared", "NavBot");
#endif // EXT_VPROF_ENABLED

	if (!IsMarkVisibleAreasAsClearedEnabled()) { return; }

	if (!threat || !threat->IsVisibleNow())
	{
		CountdownTimer& timer = GetClearAreasTimer();

		if (timer.IsElapsed())
		{
			timer.Start(CLEAR_AREAS_TIMER_INTERVAL);
			botsharedutils::search::MarkVisibleAreasAsCleared functor(GetBot<CBaseBot>());
			functor.Execute();
		}
	}
}

float ICombat::CombatData::GetTimeSinceLostLOS() const
{
	return gpGlobals->curtime - this->time_lost_los;
}

void ICombat::CombatData::Update(const CBaseBot* bot, const CKnownEntity* threat, const CBotWeapon* activeWeapon)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("ICombat::CombatData::Update", "NavBot");
#endif // EXT_VPROF_ENABLED

	CBaseEntity* entity = threat->GetEntity();
	Vector eyePos = bot->GetEyeOrigin();
	this->enemy_center = UtilHelpers::getWorldSpaceCenter(entity);
	this->in_combat = true;
	this->is_visible = threat->IsVisibleNow();
	this->enemy_range = (eyePos - this->enemy_center).Length();

	if (this->is_visible)
	{
		const WeaponInfo* info = activeWeapon->GetWeaponInfo();
		this->enemy_position = this->enemy_center;
		this->range_to_pos = (eyePos - this->enemy_position).Length();
		this->time_lost_los = gpGlobals->curtime;
		this->ten_seconds_passed = false;

		bool primary = false;
		bool secondary = false;

		if (activeWeapon->CanUsePrimaryAttack(bot) && activeWeapon->IsInAttackRange(this->range_to_pos, botweapons::AttackType::PRIMARY))
		{
			primary = true;
		}

		if (activeWeapon->CanUseSecondaryAttack(bot) && activeWeapon->IsInAttackRange(this->range_to_pos, botweapons::AttackType::SECONDARY))
		{
			secondary = true;
		}

		this->in_range = (primary || secondary);
		this->should_move = this->enemy_range >= info->GetAttackRange();

		if (primary && !secondary)
		{
			this->can_use_primary = true;
			this->selected_attack_type = botweapons::AttackType::PRIMARY;
		}
		else if (secondary && !primary)
		{
			this->can_use_primary = false;
			this->selected_attack_type = botweapons::AttackType::SECONDARY;
		}
		else if (primary && secondary)
		{
			this->can_use_primary = !CBaseBot::s_botrng.GetRandomChance(info->GetChanceToUseSecondaryAttack());
			this->selected_attack_type = this->can_use_primary ? botweapons::AttackType::PRIMARY : botweapons::AttackType::SECONDARY;
		}

		if (this->can_use_primary)
		{
			float maxrange = info->GetAttackInfo(botweapons::AttackType::PRIMARY).GetMaxRange() * info->GetHeadShotRangeMultiplier();
			this->in_headshot_range = (this->enemy_range <= maxrange);
		}
		else
		{
			float maxrange = info->GetAttackInfo(botweapons::AttackType::SECONDARY).GetMaxRange() * info->GetHeadShotRangeMultiplier();
			this->in_headshot_range = (this->enemy_range <= maxrange);
		}

		if (threat->IsPlayer())
		{
			const CBaseExtPlayer* player = threat->GetPlayerInstance();

			this->is_head_clear = bot->IsLineOfFireClear(player->GetEyeOrigin());
			this->is_center_clear = bot->IsLineOfFireClear(this->enemy_center);
			this->is_origin_clear = bot->IsLineOfFireClear(player->GetAbsOrigin());
		}
		else
		{
			const Vector& maxs = reinterpret_cast<IServerUnknown*>(entity)->GetCollideable()->OBBMaxs();
			const Vector& origin = UtilHelpers::getEntityOrigin(entity);
			Vector head = origin;
			head.z += maxs.z;

			this->is_head_clear = bot->IsLineOfFireClear(head);
			this->is_center_clear = bot->IsLineOfFireClear(this->enemy_center);
			this->is_origin_clear = bot->IsLineOfFireClear(origin);
		}

		this->can_fire = (this->is_head_clear || this->is_center_clear || this->is_origin_clear);
	}
	else
	{
		/* threat is not visible */
		this->enemy_position = threat->GetLastKnownPosition();
		this->range_to_pos = (eyePos - this->enemy_position).Length();
		/* skip these
		this->in_headshot_range = false;
		this->is_head_clear = false;
		this->is_center_clear = false;
		this->is_origin_clear = false;
		*/
		this->can_fire = false;
		this->in_range = false;
		this->should_move = true;
	}
}

combatutils::GetRandomNeighorAreaOutsideFOVFunctor::GetRandomNeighorAreaOutsideFOVFunctor(const CBaseBot* bot)
{
	m_sensor = bot->GetSensorInterface();
}

void combatutils::GetRandomNeighorAreaOutsideFOVFunctor::operator()(CNavArea* area, bool isIncomingArea)
{
	const Vector& center = area->GetCenter();

	if (!m_sensor->IsInFieldOfView(center))
	{
		m_areas.push_back(area);
	}
}

CNavArea* combatutils::GetRandomNeighorAreaOutsideFOVFunctor::GetRandomArea() const
{
	if (m_areas.empty()) { return nullptr; }
	if (m_areas.size() == 1U) { return m_areas[0]; }
	return librandom::utils::GetRandomElementFromVector(m_areas);
}
