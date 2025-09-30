#include NAVBOT_PCH_FILE
#include <string_view>
#include <extension.h>
#include <bot/basebot.h>
#include "combat.h"

#ifdef EXT_VPROF_ENABLED
#include <tier0/vprof.h>
#endif // EXT_VPROF_ENABLED

ICombat::ICombat(CBaseBot* bot) :
	IBotInterface(bot)
{
	m_lastWeaponPtr = nullptr;
	m_shouldAim = true;
	m_shouldSelectWeapons = true;
	m_isScopedOrDeployed = false;
	m_combatData.Clear();
}

ICombat::~ICombat()
{
}

void ICombat::Reset()
{
	m_lastWeaponPtr = nullptr;
	m_shouldAim = true;
	m_shouldSelectWeapons = true;
	m_isScopedOrDeployed = false;
	m_disableCombatTimer.Invalidate();
	m_dontFireTimer.Invalidate();
	m_unscopeTimer.Invalidate();
	m_useSpecialFuncTimer.Invalidate();
	m_selectWeaponTimer.Invalidate();
	m_secondaryAbilityTimer.Invalidate();
	m_combatData.Clear();
}

void ICombat::Update()
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("ICombat::Update", "NavBot");
#endif // EXT_VPROF_ENABLED

	if (!m_disableCombatTimer.IsElapsed()) { return; }

	const CBaseBot* bot = GetBot<const CBaseBot>();

	if (!bot->IsAlive()) { return; }

	const CKnownEntity* threat = bot->GetSensorInterface()->GetPrimaryKnownThreat();

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

		UpdateCombatData(threat, activeWeapon);

		if (ShouldAim())
		{
			CombatAim(bot, threat, activeWeapon);
		}

		FireWeaponAtEnemy(bot, threat, activeWeapon);
		DodgeEnemies(threat, activeWeapon);
		UseSecondaryAbilities(threat, activeWeapon);
	}
	else
	{
		m_combatData.in_combat = false;
		UnscopeWeaponIfScoped();
	}
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
	}

	return m_isScopedOrDeployed;
}

void ICombat::OnLastUsedWeaponChanged(CBaseEntity* newWeapon)
{
	m_attackTimer.Invalidate();
	m_isScopedOrDeployed = false;
	m_unscopeTimer.Invalidate();
	m_useSpecialFuncTimer.Invalidate();
}

bool ICombat::CanFireWeaponAtEnemy(const CBaseBot* bot, const CKnownEntity* threat, const CBotWeapon* activeWeapon) const
{
	if (!m_dontFireTimer.IsElapsed())
		return false;

	if (bot->GetMovementInterface()->NeedsWeaponControl())
		return false;

	if (bot->GetBehaviorInterface()->ShouldAttack(GetBot<CBaseBot>(), threat) == ANSWER_NO)
		return false;

	return true;
}

void ICombat::FireWeaponAtEnemy(const CBaseBot* bot, const CKnownEntity* threat, const CBotWeapon* activeWeapon, const bool bypassLOS)
{
	if (!CanFireWeaponAtEnemy(bot, threat, activeWeapon)) { return; }

	// Update last used weapon
	SetLastUsedWeapon(activeWeapon->GetEntity());

	const CombatData& data = GetCachedCombatData();
	IPlayerController* input = bot->GetControlInterface();
	const WeaponInfo* info = activeWeapon->GetWeaponInfo();

	if (data.can_fire || bypassLOS) // visible and has clear line of fire
	{
		if (HandleWeapon(bot, activeWeapon))
		{
			// aiming at the current enemy
			if (input->GetAimAtTarget() == threat->GetEntity())
			{
				if (input->IsAimOnTarget())
				{
					OpportunisticallyUseWeaponSpecialFunction(bot, threat, activeWeapon);
					GetAttackTimer().Start(); // just attacked

					if (data.can_use_primary)
					{
						input->PressAttackButton(info->GetAttackInfo(WeaponInfo::AttackFunctionType::PRIMARY_ATTACK).GetHoldButtonTime());
					}
					else
					{
						input->PressSecondaryAttackButton(info->GetAttackInfo(WeaponInfo::AttackFunctionType::SECONDARY_ATTACK).GetHoldButtonTime());
					}
				}
			}
		}
		else // HandleWeapon returned false
		{
			// Reloaded if needed
			if (!activeWeapon->IsLoaded())
			{
				input->PressReloadButton();
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

	if (!data.in_range)
	{
		return false; // out side firing range
	}

	// check attack interval
	if (timer.HasStarted() && timer.IsLessThen(info->GetAttackInterval()))
	{
		return false;
	}

	// no ammo in clip
	if (!activeWeapon->IsLoaded())
	{
		return false;
	}

	m_isScopedOrDeployed = activeWeapon->IsDeployedOrScoped(bot);

	if (info->NeedsToBeDeployedToFire())
	{
		if (!m_isScopedOrDeployed)
		{
			// scope/deploy
			input->PressSecondaryAttackButton(0.25f);
			GetUnscopeTimer().Start(1.0f);
			return false;
		}
	}

	return true;
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

			switch (func.button_to_press)
			{
			case INPUT_ATTACK2:

				input->PressSecondaryAttackButton(func.hold_button_time);
				break;
			case INPUT_ATTACK3:
				input->PressSpecialAttackButton(func.hold_button_time);
				break;
			case INPUT_RELOAD:
				input->PressReloadButton(func.hold_button_time);
				break;
			default:
#ifdef EXT_DEBUG
				META_CONPRINTF("Bot Weapon \"%s\" special function with invalid button to press! \n", weapon->GetWeaponInfo()->GetConfigEntryName());
#endif // EXT_DEBUG
				break;
			}
		}
	}
}

bool ICombat::IsAbleToDodgeEnemies(const CKnownEntity* threat, const CBotWeapon* activeWeapon)
{
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

	return true;
}

void ICombat::DodgeEnemies(const CKnownEntity* threat, const CBotWeapon* activeWeapon)
{
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

void ICombat::UnscopeWeaponIfScoped()
{
	if (!m_unscopeTimer.IsElapsed()) { return; }

	CBaseBot* bot = GetBot<CBaseBot>();
	const CBotWeapon* activeWeapon = bot->GetInventoryInterface()->GetActiveBotWeapon();

	if (!activeWeapon) { return; }

	m_isScopedOrDeployed = activeWeapon->IsDeployedOrScoped(bot);

	if (m_isScopedOrDeployed)
	{
		bot->GetControlInterface()->PressSecondaryAttackButton(0.2f);
		m_unscopeTimer.Start(1.0f); // check again after 1 second
	}
	else
	{
		// stop checking, this timer gets reset when the bot changes weapon or scopes during combat
		m_unscopeTimer.Start(1e6f);
	}
	
}

void ICombat::OnStartCombat(const CKnownEntity* threat, const CBotWeapon* activeWeapon)
{
	const float time = activeWeapon->GetWeaponInfo()->GetInitialAttackDelay();

	if (time > 0.0f)
	{
		DisableFiringWeapons(time, true);
	}
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

void ICombat::CombatAim(const CBaseBot* bot, const CKnownEntity* threat, const CBotWeapon* activeWeapon)
{
	using namespace std::literals::string_view_literals;

	const CombatData& data = GetCachedCombatData();
	const DifficultyProfile* profile = bot->GetDifficultyProfile();
	IPlayerController* input = bot->GetControlInterface();
	ISensor* sensor = bot->GetSensorInterface();
	constexpr auto visible_priority = IPlayerController::LookPriority::LOOK_COMBAT;
	constexpr auto notvisible_priority = IPlayerController::LookPriority::LOOK_SEARCH;
	constexpr auto aimat_duration = 1.0f;
	constexpr auto combat_aim_reason = "Looking at visible threat!"sv;
	constexpr auto search_aim_reason = "Searching for threat!"sv;

	if (data.is_visible)
	{
		const bool canHeadshot = (profile->IsAllowedToHeadshot() && data.is_head_clear);

		if (canHeadshot)
		{
			input->SetDesiredAimSpot(IDecisionQuery::DesiredAimSpot::AIMSPOT_HEAD);
			input->AimAt(threat->GetEntity(), visible_priority, aimat_duration, combat_aim_reason.data());
		}
		else if (data.is_center_clear)
		{
			input->SetDesiredAimSpot(IDecisionQuery::DesiredAimSpot::AIMSPOT_CENTER);
			input->AimAt(threat->GetEntity(), visible_priority, aimat_duration, combat_aim_reason.data());
		}
		else if (data.is_origin_clear)
		{
			input->SetDesiredAimSpot(IDecisionQuery::DesiredAimSpot::AIMSPOT_ABSORIGIN);
			input->AimAt(threat->GetEntity(), visible_priority, aimat_duration, combat_aim_reason.data());
		}
		else
		{
			// just keep aiming at the center
			input->SetDesiredAimSpot(IDecisionQuery::DesiredAimSpot::AIMSPOT_CENTER);
			input->AimAt(threat->GetEntity(), visible_priority, aimat_duration, combat_aim_reason.data());
		}
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

void ICombat::CombatData::Update(const CBaseBot* bot, const CKnownEntity* threat, const CBotWeapon* activeWeapon)
{
	CBaseEntity* entity = threat->GetEntity();
	this->enemy_center = UtilHelpers::getWorldSpaceCenter(entity);
	this->in_combat = true;

	this->is_visible = threat->IsVisibleNow();

	if (this->is_visible)
	{
		this->enemy_position = this->enemy_center;
	}
	else
	{
		this->enemy_position = threat->GetLastKnownPosition();
	}

	this->enemy_range = bot->GetRangeTo(this->enemy_center);
	this->range_to_pos = bot->GetRangeTo(this->enemy_position);
	const WeaponInfo* info = activeWeapon->GetWeaponInfo();

	bool primary = false;
	bool secondary = false;

	if (activeWeapon->CanUsePrimaryAttack(bot) && activeWeapon->IsInAttackRange(this->range_to_pos, WeaponInfo::AttackFunctionType::PRIMARY_ATTACK))
	{
		primary = true;
	}

	if (activeWeapon->CanUseSecondaryAttack(bot) && activeWeapon->IsInAttackRange(this->range_to_pos, WeaponInfo::AttackFunctionType::SECONDARY_ATTACK))
	{
		secondary = true;
	}

	this->in_range = (primary || secondary);

	if (primary && !secondary)
	{
		this->can_use_primary = true;
	}
	else if (secondary && !primary)
	{
		this->can_use_primary = false;
	}
	else if (primary && secondary)
	{
		this->can_use_primary = !CBaseBot::s_botrng.GetRandomChance(info->GetChanceToUseSecondaryAttack());
	}

	if (this->is_visible)
	{
		if (threat->IsPlayer())
		{
			const CBaseExtPlayer* player = threat->GetPlayerInstance();

			this->is_head_clear = bot->IsLineOfFireClear(player->GetEyeOrigin());
			this->is_center_clear = bot->IsLineOfFireClear(this->enemy_center);
			this->is_origin_clear = bot->IsLineOfFireClear(player->GetAbsOrigin());
		}
		else
		{
			const entities::HBaseEntity& be = threat->GetBaseEntityHelper();
			Vector maxs = be.WorldAlignMaxs();
			const Vector& origin = UtilHelpers::getEntityOrigin(entity);
			Vector head = origin;
			head.z += maxs.z;

			this->is_head_clear = bot->IsLineOfFireClear(head);
			this->is_center_clear = bot->IsLineOfFireClear(this->enemy_center);
			this->is_origin_clear = bot->IsLineOfFireClear(origin);
		}
	}
	else
	{
		this->is_head_clear = false;
		this->is_center_clear = false;
		this->is_origin_clear = bot->IsLineOfFireClear(this->enemy_position);
	}

	this->can_fire = (this->is_head_clear || this->is_center_clear || this->is_origin_clear);
}
