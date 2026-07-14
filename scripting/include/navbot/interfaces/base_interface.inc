#if defined _navbot_base_interface_included
 #endinput
#endif
#define _navbot_base_interface_included

methodmap NavBotBotInterface
{
	/**
	 * Resets the interface to it's initial state.
	 */
	public static native void Reset(Address ptr);
	// Gets the index of the bot this interface belongs to. This is guaranteed to be a valid NavBot instance.
	public static native int GetBot(Address ptr);
}