#if defined _navbot_interfaces_combat_included
 #endinput
#endif
#define _navbot_interfaces_combat_included

methodmap NavBotCombatInterface < NavBotBotInterface
{
	/**
	 * Disables the combat interface for the given time.
	 * 
	 * @param ptr			Pointer to the combat interface.
	 * @param time			How long in seconds to disable the combat interface.
	 */
	public static native void Disable(Address ptr, const float time);
	// Enables the combat interface
	public static native void Enable(Address ptr);
	/**
	 * Makes the bot fire their weapon at the given entity.
	 * This function needs to be called periodically for it to work. (100 ms intervals recommended).
	 * A timer is not needed if this is called from a scripted behavior callback or the OnPreNavBotUpdate forward.
	 * You may also want to disable the combat interface to avoid conflicts (because the combat interface will attack any visible enemies).
	 * The interface being disabled doesn't stop this function from working.
	 * 
	 * @param ptr			Pointer to the combat interface.
	 * @param entity		Entity to fire the weapon at.
	 * @param doAim			If true (default), also instructs the control interface to aim the weapon at the given entity.
	 * 
	 * @note This function does not handle weapon selection, the bot will fire their currently held weapon.
	 */
	public static native void FireWeaponAt(Address ptr, int entity, const bool doAim = true);
}