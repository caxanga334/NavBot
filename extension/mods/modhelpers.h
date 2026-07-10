#ifndef __NAVBOT_BASE_MODHELPERS_H_
#define __NAVBOT_BASE_MODHELPERS_H_

/**
 * @brief Interface for per-mod helper functions.
 */
class IModHelpers
{
public:
	IModHelpers();
	virtual ~IModHelpers();
	/**
	 * @brief Assigns a mod helper instance to the global singleton.
	 * 
	 * The global singleton is a unique_ptr smart pointer, passing NULL to this will cause the existing pointer to be deleted.
	 * @param mh Pointer to instance. NULL is valid.
	 */
	static void SetInstance(IModHelpers* mh);
	/**
	 * @brief Gets the entity team number.
	 * @param entity Entity to get the team.
	 * @return Team number of the given entity.
	 */
	virtual int GetEntityTeamNumber(CBaseEntity* entity) const;
	/**
	 * @brief Checks if the given entity is a player.
	 * @param entity Entity to check.
	 * @return True if the given entity is a player, false otherwise.
	 */
	virtual bool IsPlayer(CBaseEntity* entity) const;
	/**
	 * @brief Checks if the entity is alive.
	 * @param entity Entity to check.
	 * @return Returns true if the entity is alive, false otherwise.
	 */
	virtual bool IsAlive(CBaseEntity* entity) const;
	/**
	 * @brief Checks if the entity is dead.
	 * @param entity Entity to check.
	 * @return Returns true if the entity is dead, false otherwise.
	 */
	bool IsDead(CBaseEntity* entity) const { return !IsAlive(entity); }
	/**
	 * @brief Checks if the given entity is a combat character (IE: players and npcs).
	 * @param entity Entity to check.
	 * @return True if given entity is a combat character, false otherwise.
	 */
	virtual bool IsCombatCharacter(CBaseEntity* entity) const;
	/**
	 * @brief Gets the entity's current water level.
	 * @param entity Entity to read the water level.
	 * @return Entity's water level.
	 */
	virtual int GetEntityWaterLevel(CBaseEntity* entity) const;
	/**
	 * @brief Determines if the given entity is being rendered and is visible.
	 * @param entity Entity to check.
	 * @return True if the entity is visible, false otherwise.
	 */
	virtual bool IsEntityVisible(CBaseEntity* entity) const;
	/**
	 * @brief Determines if the given player can pick up the item.
	 * @param item Item being checked.
	 * @param player Player that wants to pick the item.
	 * @return Returns true if the player can pick the item, false otherwise.
	 */
	virtual bool CanPlayerPickUpItem(CBaseEntity* item, CBaseEntity* player) const;
	/**
	 * @brief Check if this is a playable team.
	 * @param teamNum Team number to check.
	 * @return True if yes, false otherwise.
	 */
	virtual bool IsPlayableTeam(int teamNum) const;
	static constexpr int NO_OPPOSING_TEAM = -1;
	/**
	 * @brief Given a team index, returns the team index of the opposing team.
	 * @param teamNum Team to get the opposing team of.
	 * @return Opposing team index or NO_OPPOSING_TEAM if an opposing team cannot be determined.
	 */
	virtual int GetOpposingTeamIndex(int teamNum) const;
	/**
	 * @brief Tests if the given entities passed the filter.
	 * @param filter Filter entity.
	 * @param caller Caller (can be NULL).
	 * @param activator Activator.
	 * @return True if the activator passes the filter, false otherwise.
	 */
	virtual bool PassesFilterImpl(CBaseEntity* filter, CBaseEntity* caller, CBaseEntity* activator) const;
	/**
	 * @brief Determines if there is an obstruction that blocks +USE for the given entity.
	 * @param player Player using the entity.
	 * @param entity Entity being used.
	 * @param obstruction Optional. The entity blocking +USE.
	 * @return true if an obstructed is found, false otherwise.
	 */
	virtual bool IsUseObstructed(CBaseEntity* player, CBaseEntity* entity, CBaseEntity** obstruction = nullptr) const;
	static constexpr int NO_ECON_INDEX = -1; // constant to represent an invalid/no econ index
	/**
	 * @brief Given an item, gets the economy index if it has one.
	 * @param item Item to get the economy index of.
	 * @return Item index or NO_ECON_INDEX if not available.
	 */
	virtual int GetItemEconomyIndex(CBaseEntity* item) const { return NO_ECON_INDEX; }
	/**
	 * @brief Called to check if the given entity can take damage. This function checks for common things like the takedamage property.
	 * @param entity Entity to check.
	 * @param maxhealth If the entity's health is greater than this, consider as not damagable.
	 * @return True if the given entity can take damage, false if not.
	 */
	virtual bool IsEntityDamageable(CBaseEntity* entity, const int maxhealth = 1000) const;
	/**
	 * @brief Called to check if the given entity can be damaged by the other entity.
	 * @param entity Entity to check.
	 * @param attacker Entity that wants to deal damage.
	 * @return True if the given attacker can deal damage to the entity, false otherwise.
	 */
	virtual bool IsEntityDamageableBy(CBaseEntity* entity, CBaseEntity* attacker) const;
	/**
	 * @brief Called to check if the given entity is a breakable entity. Bots will attack these if they find one in their path.
	 * @param entity Entity to check.
	 * @return True if the given entity is a brakable entity, false if not.
	 */
	virtual bool IsEntityBreakable(CBaseEntity* entity) const;
	/**
	 * @brief Called to check if the given breakable entity is already broken.
	 * Some entities break but aren't deleted. This should return true when the entity is broken and no longer solid for players.
	 * @param entity Entity being checked.
	 * @return True if the entity is broken and passable, false otherwise.
	 */
	virtual bool IsBreakableBroken(CBaseEntity* entity) const;
	/**
	 * @brief Queries if the given entity explodes when damaged/broken.
	 * @param entity Entity being queried.
	 * @return True if explosive, false if not.
	 */
	virtual bool IsBreakableExplosive(CBaseEntity* entity) const;
};

// Global singleton to access the mod helpers interface
extern std::unique_ptr<IModHelpers> modhelpers;

#endif // !__NAVBOT_BASE_MODHELPERS_H_
