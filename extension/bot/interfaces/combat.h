#ifndef __NAVBOT_BOT_COMBAT_INTERFACE_H_
#define __NAVBOT_BOT_COMBAT_INTERFACE_H_

#include "base_interface.h"
#include <bot/interfaces/decisionquery.h>
#include <bot/interfaces/weapons_shared.h>

/**
 * @brief The combat interface. Responsible for the basic handling of combat such as firing, reloading weapons, using abilities, etc.
 */
class ICombat : public IBotInterface
{
public:
	ICombat(CBaseBot* bot);
	~ICombat() override;

	static constexpr float CALLOUT_COOLDOWN = 10.0f;
	static constexpr float LOOK_AROUND_BASE_DURATION = 1.5f;
	static constexpr float LOOK_AROUND_TIMER_BASE_MIN = 3.0f;
	static constexpr float LOOK_AROUND_TIMER_BASE_MAX = 7.0f;
	static constexpr int LOOK_AROUND_MIN_SKILL = 25;
	static constexpr float POST_COMBAT_TIMER_DURATION = 5.0f;
	static constexpr int DANGER_SCAN_IGNORE_FOV_SKILL = 90;
	static constexpr float TOGGLE_SCOPE_COOLDOWN_TIME = 0.5f;
	static constexpr float CLEAR_AREAS_TIMER_INTERVAL = 3.0f;

	struct CombatData
	{
		float enemy_range; // distance between the bot and the enemy's world space center
		float range_to_pos; // distance between the bot and the selected enemy position
		Vector enemy_position; // enemy position (center or LKP)
		Vector enemy_center; // enemy world space center
		bool in_combat; // currently in combat
		bool is_visible; // enemy is currently visible
		bool is_head_clear; // clear line of fire to head
		bool is_center_clear; // clear line of fire to center
		bool is_origin_clear; //clear line of fire to abs origin
		bool in_range; // enemy is is range of at least one attack type
		bool in_headshot_range; // in range of a headshot attack
		bool can_use_primary; // can use primary attack
		bool can_fire; // true if has a clear LOF
		bool should_move; // for behaviors, if true, the bot should move towards the threat
		bool ten_seconds_passed; // 10 seconds have passed since any threat was visible
		float time_lost_los; // timestamp of when LOS was lost
		botweapons::AttackType selected_attack_type; // currently selected attack type

		// Returns the amount of seconds passed since the bot lost line of sight with the current enemy.
		float GetTimeSinceLostLOS() const;
		// Returns true if the current threat is visible but the bot doesn't have a clear line of fire
		bool IsVisibleButNoClearLOF() const { return is_visible && !can_fire; }
		// Returns true if the bot should path towards the current enemy.
		bool ShouldPathToEnemy() const { return should_move || !in_range || IsVisibleButNoClearLOF(); }

		void Clear()
		{
			enemy_range = 0.0f;
			range_to_pos = 0.0f;
			enemy_position.Init();
			enemy_center.Init();
			in_combat = false;
			is_visible = false;
			is_head_clear = false;
			is_center_clear = false;
			is_origin_clear = false;
			in_range = false;
			in_headshot_range = false;
			can_use_primary = false;
			can_fire = false;
			should_move = false;
			ten_seconds_passed = false;
			time_lost_los = -9999.0f;
			selected_attack_type = botweapons::AttackType::MAX_ATTACK_TYPES;
		}

		void Update(const CBaseBot* bot, const CKnownEntity* threat, const CBotWeapon* activeWeapon);
	};

	void OnNavAreaChanged(CNavArea* oldArea, CNavArea* newArea) override;
	void Reset() override;
	void Update() override;
	void Frame() override;

	/**
	 * @brief Returns true if the given weapon is the last used weapon in combat.
	 * @param other Weapon to compare.
	 * @return True if it's the last used, false otherwise.
	 */
	bool IsLastUsedWeapon(CBaseEntity* other) const { return other == m_lastWeaponPtr; }
	/**
	 * @brief Disables combat for the given duration.
	 * @param duration How long to keep combat disabled in seconds.
	 */
	void DisableCombat(const float duration) { m_disableCombatTimer.Start(duration); }
	// Enables combat
	void EnableCombat() { m_disableCombatTimer.Invalidate(); }
	/**
	 * @brief Sets if the combat interface should handle aiming weapons at enemies.
	 * @param state True to enable, false to disable.
	 */
	void SetAimState(bool state) { m_shouldAim = state; }
	// Returns true if the combat interface should handle aiming at enemies.
	const bool ShouldAim() const { return m_shouldAim; }
	/**
	 * @brief Sets if the combat interface should handle weapon selection.
	 * @param state True to enable, false to disable.
	 */
	void SetSelectWeaponState(bool state) { m_shouldSelectWeapons = state; }
	// Returns true if the combat interface should handle weapon selection.
	const bool ShouldSelectWeapons() const { return m_shouldSelectWeapons; }
	// Returns true if the bot is currently in combat.
	const bool IsInCombat() const { return m_combatData.in_combat; }
	// Returns the cached combat data, may be outdated.
	const CombatData& GetCachedCombatData() const { return m_combatData; }
	// Returns if the bot is scoped ir or with their weapon deployed (cached information)
	const bool IsScopedInOrDeployed() const { return m_isScopedOrDeployed; }
	/**
	 * @brief Makes the combat interface stop firing weapons for the given time.
	 * @param time How long in seconds to stop firing.
	 * @param letGoOfButtons If true, also tell the control interface to release all attack buttons.
	 */
	void DisableFiringWeapons(const float time, const bool letGoOfButtons = false);
	// Enables the firing of weapons.
	void EnableFiringWeapons() { m_dontFireTimer.Invalidate(); }
	/**
	 * @brief Fires the currently held weapon at the given known entity.
	 * @param entity Entity to fire at.
	 * @param doAim If true, will also handle aiming.
	 * @return True if the weapon was fired.
	 */
	void FireWeaponAt(CBaseEntity* entity, const bool doAim = true);
	/**
	 * @brief Tells the combat interface to scope in/deploy the currently held weapon.
	 * @return True if the weapon is currently scoped in/deployed, false otherwise.
	 */
	bool ScopeInOrDeployWeapon();
	/**
	 * @brief Sets the wants to scope state.
	 * @param state If true, the bot will try to use the scope, if false, the bot will try to unscope.
	 */
	void SetWantsToScopeState(bool state) { m_wantsToScope = state; }
	// Returns true if the bot wants to use their weapon's scope, false otherwise.
	bool WantstoBeScoped() const { return m_wantsToScope; }
	// Returns true if the given threat is the last one the bot was in combat with.
	const bool IsLastThreat(const CKnownEntity* threat) const { return threat != nullptr && threat == m_lastThreatPtr; }
	// Returns how many seconds have passed since line of sight was lost to the current threat.
	const float GetTimeSinceLOSWasLost() const { return m_combatData.GetTimeSinceLostLOS(); }
	// Returns true if the bot is reaiming.
	const bool IsReAiming() const { return m_reAim; }
	// Returns the index of the last place reported by the bot in a callout
	unsigned int GetLastReportedPlace() const { return m_lastPlace; }
	/**
	 * @brief Utility function for making the bot do a basic primary attack with the currently held weapon.
	 */
	virtual void DoPrimaryAttack();
	// Stops the bot from looking around for the given time.
	void StopLookingAround(float time = 1.0f) { m_disableLookingAroundTimer.Start(time); }
	// Allows the bot to resume looking around
	void StartLookingAround() { m_disableLookingAroundTimer.Invalidate(); }
	// Is the bot allowed to look around?
	virtual const bool CanLookAround() const;
	// Disables dodging for the given time in seconds
	void DisableDodging(float time = 1.0f) { m_disableDodgeTimer.Start(time); }
	// Enable dodging
	void EnableDodging() { m_disableDodgeTimer.Invalidate(); }
	/**
	 * @brief Sets if the bot should reload their weapon post combat.
	 * @param state State
	 */
	void SetShouldReloadPostCombat(bool state) { m_shouldReloadPostCombat = state; }
	// Returns true if the bot should reload their weapon post combat.
	const bool GetShouldReloadPostCombat() const { return m_shouldReloadPostCombat; }
	// Returns true if the current enemy is visible but the line of fire is obstructed (used cached information).
	const bool IsVisibleButLineOfFireIsObstructed() const { return m_combatData.is_visible && !m_combatData.can_fire; }
	// Returns the attack type selected to be used against the current enemy
	botweapons::AttackType GetSelectedAttackType() const { return m_combatData.selected_attack_type; }
	// Reads danger scan values from gamedata.
	static bool DangerScanParseGamedata();
	// Returns true if danger scan is enabled.
	static bool IsDangerScanEnabled() { return s_allowDangerScan; }
	// Returns true if danger scan should check the projectile's team and ignore projectiles of the same team as the bot.
	static bool IsDangerScanTeamCheckEnabled() { return s_dangerCheckTeam; }
	// Returns a set of entities that are considered a danger source.
	static const std::unordered_set<std::string>& GetDangerScanEntities() { return s_dangerEnts; }
	// Returns the entity selected as the most dangerous entity in the last danger scan update. NULL if none or if the entity is no longer valid.
	CBaseEntity* GetMostDangerousEntity() const { return m_lastDangerEntity.Get(); }
	// Returns true if the combat interface is marking visible areas as cleared of enemies.
	const bool IsMarkVisibleAreasAsClearedEnabled() const { return m_clearAreasTimer.HasStarted(); }
	// Enables marking visible areas as cleared.
	void StartMarkingVisibleAreasAsCleared() { m_clearAreasTimer.Start(-1.0f); /* Start with a negative time so it runs in the next update cycle. */ }
	// Disables marking visible areas as cleared.
	void StopMarkingVisibleAreasAsCleared() { m_clearAreasTimer.Invalidate(); }
protected:
	/**
	 * @brief Called when the last used weapon in combat has changed.
	 * @param newWeapon The new last used weapon.
	 */
	virtual void OnLastUsedWeaponChanged(CBaseEntity* newWeapon);
	/**
	 * @brief Sets the last used weapon by the bot in combat.
	 * 
	 * Will call OnLastUsedWeaponChanged.
	 * @param weapon Weapon's CBaseEntity pointer.
	 */
	void SetLastUsedWeapon(CBaseEntity* weapon)
	{
		if (!weapon) { m_lastWeaponPtr = weapon; return; }

		if (weapon != m_lastWeaponPtr) { OnLastUsedWeaponChanged(weapon); }

		m_lastWeaponPtr = weapon;
	}

	// Aims the weapon while in the combat state
	virtual void CombatAim(const CBaseBot* bot, const CKnownEntity* threat, const CBotWeapon* activeWeapon);
	// Returns true if it's possible to fire the bot's current active weapon
	virtual bool CanFireWeaponAtEnemy(const CBaseBot* bot, const CKnownEntity* threat, const CBotWeapon* activeWeapon) const;
	/**
	 * @brief Tries to fire a weapon at the given enemy. (Internal Version)
	 * @param bot Bot that will be firing the weapon.
	 * @param threat The current enemy.
	 * @param activeWeapon The weapon to fire.
	 * @param bypassLOS If true, allow firing the weapon even if the enemy is obstructed.
	 */
	virtual void FireWeaponAtEnemy(const CBaseBot* bot, const CKnownEntity* threat, const CBotWeapon* activeWeapon, const bool bypassLOS = false);
	// Invoked to handle the usage of the given weapon. Return true to allow the bot to fire the weapon.
	virtual bool HandleWeapon(const CBaseBot* bot, const CBotWeapon* activeWeapon);
	/**
	 * @brief Invoked when handle weapon returns false.
	 * @param bot Bot that will be firing the weapon.
	 * @param threat The current enemy.
	 * @param activeWeapon The weapon to fire.
	 */
	virtual void OnHandleWeaponFailed(const CBaseBot* bot, const CKnownEntity* threat, const CBotWeapon* activeWeapon);
	virtual void ReloadCurrentWeapon(const CBaseBot* bot, const CBotWeapon* activeWeapon);
	void UpdateCombatData(const CKnownEntity* threat, const CBotWeapon* activeWeapon) 
	{
		botweapons::AttackType lastUsedAttackType = m_combatData.selected_attack_type;
		m_combatData.Update(GetBot<CBaseBot>(), threat, activeWeapon);

		if (botweapons::IsValidAttackType(lastUsedAttackType) && 
			botweapons::IsValidAttackType(m_combatData.selected_attack_type) &&
			lastUsedAttackType != m_combatData.selected_attack_type)
		{
			OnSelectedAttackTypeChanged(threat, activeWeapon, lastUsedAttackType);
		}
	}
	/**
	 * @brief Call to make bots use the weapon's special function
	 * @param bot Bot that is using the weapon.
	 * @param threat Current enemy for information.
	 * @param weapon The weapon itself.
	 */
	virtual void OpportunisticallyUseWeaponSpecialFunction(const CBaseBot* bot, const CKnownEntity* threat, const CBotWeapon* weapon);
	IntervalTimer& GetAttackTimer() { return m_attackTimer; }
	// Timer used for toggling between scope/unscoped mode.
	CountdownTimer& GetToggleScopeTimer() { return m_toggleScopeTimer; }
	// Timer used for special weapon functions
	CountdownTimer& GetSpecialWeaponFunctionTimer() { return m_useSpecialFuncTimer; }
	CountdownTimer& GetWeaponSelectionTimer() { return m_selectWeaponTimer; }
	CountdownTimer& GetScopeInDelayTimer() { return m_scopeinDelayTimer; }
	// Sets the scoped/deployed status.
	void SetScopedOrDeployedStatus(bool status) { m_isScopedOrDeployed = status; }
	/**
	 * @brief Returns true if the bot is allowed to dodge in combat.
	 * @param threat Current enemy. NULL is not accepted and will crash!
	 * @param activeWeapon Current weapon being used. NULL is not accepted and will crash!
	 * @return True if dodging is possible, false otherwise.
	 */
	virtual bool IsAbleToDodgeEnemies(const CKnownEntity* threat, const CBotWeapon* activeWeapon);
	// returns if the bot is allowed to jump or crouch while dodging enemies in combat.
	virtual bool CanJumpOrCrouchToDodge() const { return true; }
	/**
	 * @brief Called to make bots dodges enemies.
	 * @param threat Current enemy.
	 * @param activeWeapon Current weapon.
	 */
	virtual void DodgeEnemies(const CKnownEntity* threat, const CBotWeapon* activeWeapon);
	/**
	 * @brief Called when the bot initially enters combat with an enemy.
	 * @param threat Current enemy.
	 * @param activeWeapon Current weapon.
	 */
	virtual void OnStartCombat(const CKnownEntity* threat, const CBotWeapon* activeWeapon);
	/**
	 * @brief Called when the bot's current threat changes.
	 * @param threat New threat.
	 */
	virtual void OnNewThreat(const CKnownEntity* threat) {};
	/**
	 * @brief Called when the bot's current threat becomes visible
	 * @param threat Current threat.
	 * @param activeWeapon Current weapon.
	 */
	virtual void OnThreatBecameVisible(const CKnownEntity* threat, const CBotWeapon* activeWeapon);
	/**
	 * @brief Called when the selected weapon attack type changes. (IE: primary to secondary)
	 * @param threat Current enemy being attacked.
	 * @param activeWeapon Current weapon being used.
	 * @param oldtype The old attack type being used. The current attack type is stored in the combat data.
	 */
	virtual void OnSelectedAttackTypeChanged(const CKnownEntity* threat, const CBotWeapon* activeWeapon, botweapons::AttackType oldtype);
	// Returns true if the bot can use secondary abilities
	virtual bool CanUseSecondaryAbilitities() const;
	/**
	 * @brief Opportunistically use secondary weapon and non-weapon abilities. (Mod specific)
	 * @param threat Current enemy.
	 * @param activeWeapon Current weapon.
	 */
	virtual void UseSecondaryAbilities(const CKnownEntity* threat, const CBotWeapon* activeWeapon) {}
	CountdownTimer& GetSecondaryAbilitiesTimer() { return m_secondaryAbilityTimer; } // Timer for secondary abilities
	/**
	 * @brief Sets the last combat threat.
	 * @param threat Threat to set. NULL to clear.
	 */
	void SetLastThreat(const CKnownEntity* threat)
	{
		if (threat == nullptr)
		{
			m_lastThreatPtr = nullptr;
			return;
		}

		if (m_lastThreatPtr != threat)
		{
			OnNewThreat(threat);
			m_lastThreatPtr = threat;
		}
	}
	// Sets the ReAim variable
	void SetReAim(bool status) { m_reAim = status; }
	/**
	 * @brief Checks if the bot can send a callout of a threat.
	 * @param threat Current threat.
	 * @return true if possible, false otherwise.
	 */
	virtual bool CanCalloutThreat(const CKnownEntity* threat) const;
	/**
	 * @brief Try to callout a threat.
	 * @param threat Current threat.
	 */
	virtual void TryToCalloutThreat(const CKnownEntity* threat);
	/**
	 * @brief Formats and sends the actual callout.
	 * @param threat Current threat.
	 */
	virtual void SendCalloutOfTreat(const CKnownEntity* threat);
	// gets the callout cooldown timer
	CountdownTimer& GetCalloutTimer() { return m_calloutTimer; }
	/**
	 * @brief Returns the enemy name to be sent in chat for callouts.
	 * 
	 * This function must return a valid string. NULL is NOT allowed.
	 * @param enemy Enemy to get the name from
	 * @return Enemy name string.
	 */
	virtual const char* GetEnemyName(const CKnownEntity* enemy) const;
	// Sets the last reported place name index.
	void SetLastReportedPlace(unsigned int place) { m_lastPlace = place; }
	// Timer for danger scans.
	CountdownTimer& GetDangerScanTimer() { return m_dangerScanTimer; }
	// Timer for weapon reloads.
	CountdownTimer& GetReloadTimer() { return m_reloadTimer; }
	// Timer for look around.
	CountdownTimer& GetLookAroundTimer() { return m_lookAroundTimer; }
	// Timer for disabling look round.
	CountdownTimer& GetDisableLookRoundTimer() { return m_disableLookingAroundTimer; }
	// Timer for disabling dodge.
	CountdownTimer& GetDisableDodgeTimer() { return m_disableDodgeTimer; }
	// Timer for post combat checks.
	CountdownTimer& GetPostCombatChecksTimer() { return m_postCombatChecks; }
	// Timer to block firing the weapon.
	CountdownTimer& GetBlockFiringTimer() { return m_blockFiringTimer; }
	// Timer for marking nav areas as cleared
	CountdownTimer& GetClearAreasTimer() { return m_clearAreasTimer; }
	// Invoked to update the look around logic.
	virtual void UpdateLookingAround();
	/**
	 * @brief Called after a bot has left combat.
	 */
	virtual void OnPostCombat();
	// Returns true if the bot is allowed to scan for danger.
	virtual bool CanScanForDanger() const;
	/**
	 * @brief Called to determine if the given entity should be classified as a danger source to the bot.
	 * @param entity Entity being checked.
	 * @return True if a danger, false otherwise.
	 */
	virtual bool IsEntityADangerSource(CBaseEntity* entity) const;
	/**
	 * @brief Called to determine if the bot should be aware of this danger entity (IE: has clear line of sight).
	 * @param entity Entity being checked.
	 * @return True if the bot should be aware, false if not.
	 */
	virtual bool IsAwareOfDangerEntity(CBaseEntity* entity) const;
	/**
	 * @brief Called to filter dangerous entities and select the most dangerous one.
	 * @param first First entity choice.
	 * @param second Second entity choice.
	 * @return Selected entity. Returning NULL will end the search and set the most dangerous entity to NULL.
	 */
	virtual CBaseEntity* SelectMostDangerousEntity(CBaseEntity* first, CBaseEntity* second) const;
	/**
	 * @brief Sets the most dangerous entity.
	 * @param entity Entity to store.
	 */
	void SetMostDangerousEntity(CBaseEntity* entity) { m_lastDangerEntity = entity; }
	/**
	 * @brief Called after ~10 seconds have passed since the last threat was visible.
	 */
	virtual void OnTenSecondsSinceThreatVisible();
private:
	CBaseEntity* m_lastWeaponPtr;
	const CKnownEntity* m_lastThreatPtr;
	CountdownTimer m_dangerScanTimer;
	CountdownTimer m_disableCombatTimer;
	CountdownTimer m_dontFireTimer;
	CountdownTimer m_toggleScopeTimer;
	CountdownTimer m_useSpecialFuncTimer;
	CountdownTimer m_selectWeaponTimer; // cooldown for weapon selection
	CountdownTimer m_secondaryAbilityTimer;
	CountdownTimer m_scopeinDelayTimer;
	CountdownTimer m_calloutTimer;
	CountdownTimer m_reloadTimer;
	CountdownTimer m_disableLookingAroundTimer;
	CountdownTimer m_lookAroundTimer;
	CountdownTimer m_disableDodgeTimer;
	CountdownTimer m_postCombatChecks;
	CountdownTimer m_blockFiringTimer; // timer used for cooldown between attacks (shared)
	CountdownTimer m_clearAreasTimer; // time used for the cooldown between marking visible nav areas as cleared
	IntervalTimer m_attackTimer; // timer used for cooldown between attacks (not shared)
	CombatData m_combatData;
	bool m_shouldAim;
	bool m_shouldSelectWeapons;
	bool m_isScopedOrDeployed;
	bool m_wantsToScope; // true if the bot wants to use a scope, false otherwise
	bool m_reAim;
	bool m_shouldReloadPostCombat;
	unsigned int m_lastPlace;
	CHandle<CBaseEntity> m_lastDangerEntity; // last selected danger entity.

	IDecisionQuery::DesiredAimSpot SelectClearAimSpot(const bool allowheadshots) const;

	/* Danger Scan */
	static inline bool s_allowDangerScan{ true };
	static inline bool s_dangerCheckTeam{ true };
	static inline std::unordered_set<std::string> s_dangerEnts{};

	void DangerScanUpdate(); // runs danger scan logic
	void UpdateScopeState();
	void UpdateMarkAreasAsCleared(const CKnownEntity* threat);
};

namespace combatutils
{
	class GetRandomNeighorAreaOutsideFOVFunctor
	{
	public:
		GetRandomNeighorAreaOutsideFOVFunctor(const CBaseBot* bot);

		void operator()(CNavArea* area, bool isIncomingArea);

		CNavArea* GetRandomArea() const;

	private:
		const ISensor* m_sensor;
		std::vector<CNavArea*> m_areas;
	};

	/**
	 * @brief Utility class for toggle the combat interface area clearing feature. The destructor will automatically disable it.
	 */
	class ToggleAreaClearing
	{
	public:
		ToggleAreaClearing(ICombat* combat = nullptr)
		{
			m_combat = combat;

			if (combat)
			{
				combat->StartMarkingVisibleAreasAsCleared();
			}
		}

		~ToggleAreaClearing()
		{
			if (m_combat)
			{
				m_combat->StopMarkingVisibleAreasAsCleared();
				m_combat = nullptr;
			}
		}

		void Enable(ICombat* combat)
		{
			m_combat = combat;
			combat->StartMarkingVisibleAreasAsCleared();
		}

		void Disable()
		{
			if (m_combat)
			{
				m_combat->StopMarkingVisibleAreasAsCleared();
				m_combat = nullptr;
			}
		}

		bool IsEnabled() const { return m_combat != nullptr; }

	private:
		ICombat* m_combat;
	};
}

#endif // !__NAVBOT_BOT_COMBAT_INTERFACE_H_
