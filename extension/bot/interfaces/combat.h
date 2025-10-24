#ifndef __NAVBOT_BOT_COMBAT_INTERFACE_H_
#define __NAVBOT_BOT_COMBAT_INTERFACE_H_

#include "base_interface.h"
#include <bot/interfaces/decisionquery.h>

/**
 * @brief The combat interface. Responsible for the basic handling of combat such as firing, reloading weapons, using abilities, etc.
 */
class ICombat : public IBotInterface
{
public:
	ICombat(CBaseBot* bot);
	~ICombat() override;

	static constexpr float CALLOUT_COOLDOWN = 10.0f;

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
		float time_lost_los; // timestamp of when LOS was lost

		// Returns the amount of seconds passed since the bot lost line of sight with the current enemy.
		float GetTimeSinceLostLOS() const;

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
			time_lost_los = -9999.0f;
		}

		void Update(const CBaseBot* bot, const CKnownEntity* threat, const CBotWeapon* activeWeapon);
	};

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
	// Tells the combat interface to stop scoping in with the current weapon.
	void UnScopeWeapon() { m_unscopeTimer.Start(1.0f); }
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
	void UpdateCombatData(const CKnownEntity* threat, const CBotWeapon* activeWeapon) { m_combatData.Update(GetBot<CBaseBot>(), threat, activeWeapon); }
	/**
	 * @brief Call to make bots use the weapon's special function
	 * @param bot Bot that is using the weapon.
	 * @param threat Current enemy for information.
	 * @param weapon The weapon itself.
	 */
	virtual void OpportunisticallyUseWeaponSpecialFunction(const CBaseBot* bot, const CKnownEntity* threat, const CBotWeapon* weapon);

	IntervalTimer& GetAttackTimer() { return m_attackTimer; }
	CountdownTimer& GetUnscopeTimer() { return m_unscopeTimer; }
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

	virtual void DodgeEnemies(const CKnownEntity* threat, const CBotWeapon* activeWeapon);
	void UnscopeWeaponIfScoped();
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
	// Sets the last reported place name index
	void SetLastReportedPlace(unsigned int place) { m_lastPlace = place; }
	// Timer for weapon reloads
	CountdownTimer& GetReloadTimer() { return m_reloadTimer; }
private:
	CBaseEntity* m_lastWeaponPtr;
	const CKnownEntity* m_lastThreatPtr;
	CountdownTimer m_disableCombatTimer;
	CountdownTimer m_dontFireTimer;
	CountdownTimer m_unscopeTimer;
	CountdownTimer m_useSpecialFuncTimer;
	CountdownTimer m_selectWeaponTimer; // cooldown for weapon selection
	CountdownTimer m_secondaryAbilityTimer;
	CountdownTimer m_scopeinDelayTimer;
	CountdownTimer m_calloutTimer;
	CountdownTimer m_reloadTimer;
	IntervalTimer m_attackTimer;
	CombatData m_combatData;
	bool m_shouldAim;
	bool m_shouldSelectWeapons;
	bool m_isScopedOrDeployed;
	bool m_reAim;
	unsigned int m_lastPlace;

	IDecisionQuery::DesiredAimSpot SelectClearAimSpot(const bool allowheadshots) const;
};

#endif // !__NAVBOT_BOT_COMBAT_INTERFACE_H_
