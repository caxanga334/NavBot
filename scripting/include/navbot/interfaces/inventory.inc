#if defined _navbot_interfaces_inventory_included
 #endinput
#endif
#define _navbot_interfaces_inventory_included

methodmap NavBotInventoryInterface < NavBotBotInterface
{
	/**
	 * Makes the bot switch to the given weapon.
	 * 
	 * @param weapon		Entity index of the weapon to equip.
	 * @return				Returns true if the weapon was equipped.
	 * @error				Invalid weapon or the bot doesn't own the given weapon.
	 */
	public native bool EquipWeapon(int weapon);
	/**
	 * Registers the given weapon entity into the bot's inventory. Call this if you're giving weapons to the bot via plugins.
	 * 
	 * @param weapon		Entity index of the weapon to equip.
	 * @error				Invalid weapon entity.
	 */
	public native void RegisterWeapon(int weapon);
	/**
	 * Force the inventory interface to update the internal list of weapons the bot owns.
	 */
	public native void RequestUpdate();
}