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
	virtual int GetEntityTeamNumber(CBaseEntity* entity);
	/**
	 * @brief Checks if the given entity is a player.
	 * @param entity Entity to check.
	 * @return True if the given entity is a player, false otherwise.
	 */
	virtual bool IsPlayer(CBaseEntity* entity);
	/**
	 * @brief Checks if the entity is alive.
	 * @param entity Entity to check.
	 * @return Returns true if the entity is alive, false otherwise.
	 */
	virtual bool IsAlive(CBaseEntity* entity);
	/**
	 * @brief Checks if the entity is dead.
	 * @param entity Entity to check.
	 * @return Returns true if the entity is dead, false otherwise.
	 */
	bool IsDead(CBaseEntity* entity) { return !IsAlive(entity); }
	/**
	 * @brief Checks if the given entity is a combat character (IE: players and npcs).
	 * @param entity Entity to check.
	 * @return True if given entity is a combat character, false otherwise.
	 */
	virtual bool IsCombatCharacter(CBaseEntity* entity);
};

// Global singleton to access the mod helpers interface
extern std::unique_ptr<IModHelpers> modhelpers;

#endif // !__NAVBOT_BASE_MODHELPERS_H_
