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
}