#if defined _navbot_interfaces_inventory_included
 #endinput
#endif
#define _navbot_interfaces_inventory_included

// Interface responsible for mananing the bot's inventory (of weapons).
methodmap NavBotInventoryInterface < NavBotBotInterface
{
	/**
	 * Makes the bot switch to the given weapon.
	 * 
	 * @param ptr			Pointer to the inventory interface (that you get via NavBot.GetInventoryInterface())
	 * @param weapon		Entity index of the weapon to equip.
	 * @return				Returns true if the weapon was equipped.
	 * @error				Invalid weapon or the bot doesn't own the given weapon.
	 */
	public static native bool EquipWeapon(Address ptr, int weapon);
	/**
	 * Registers the given weapon entity into the bot's inventory. Call this if you're giving weapons to the bot via plugins.
	 * 
	 * @param ptr			Pointer to the inventory interface (that you get via NavBot.GetInventoryInterface())
	 * @param weapon		Entity index of the weapon to equip.
	 * @error				Invalid weapon entity.
	 */
	public static native void RegisterWeapon(Address ptr, int weapon);
	/**
	 * Force the inventory interface to update the internal list of weapons the bot owns.
	 * 
	 * @param ptr			Pointer to the inventory interface (that you get via NavBot.GetInventoryInterface())
	 */
	public static native void RequestUpdate(Address ptr);
	/**
	 * Makes the bot equip the first weapon that contains the given tag.
	 * 
	 * @param ptr			Pointer to the inventory interface (that you get via NavBot.GetInventoryInterface())
	 * @param tag			Tag the weapon must have. (Tags are defined in the weapon config file)
	 * @return				True if the weapon was equipped, false otherwise.
	 */
	public static native bool SelectFirstWeaponWithTag(Address ptr, const char[] tag);
	// Selects the best weapon the bot owns.
	public static native void SelectBestWeapon(Address ptr);
}