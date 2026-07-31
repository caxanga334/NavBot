#if defined _navbot_modhelpers_interface_included
 #endinput
#endif
#define _navbot_modhelpers_interface_included

methodmap NavBotModHelpers
{
	/**
	 * Queries if the given entity is considered a breakable. Bots will break these if they encounter one in their path.
	 * 
	 * @param entity			Entity being queried.
	 * @return					True if breakable, false otherwise.
	 * @error					Invalid entity passed.
	 */
	public static native bool IsEntityBreakable(int entity);
	/**
	 * Queries if the given entity can take damage.
	 * 
	 * @param entity			Entity being queried.
	 * @param maxhealth			If the entity's health is larger than this value, consider as not damageable.
	 * @return					True if yes, false otherwise.
	 * @error					Invalid entity passed.
	 */
	public static native bool IsEntityDamageable(int entity, const int maxhealth);
	/**
	 * Queries if the given entity can be damaged by a specific attacker.
	 * 
	 * @param entity			Entity being queried.
	 * @param attacker			Entity attacking the entity passed in the first parameter.
	 * @return					True if yes, false otherwise.
	 * @error					Invalid entity passed.
	 */
	public static native bool IsEntityDamageableBy(int entity, int attacker);
}
