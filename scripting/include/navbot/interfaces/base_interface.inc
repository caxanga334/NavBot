#if defined _navbot_base_interface_included
 #endinput
#endif
#define _navbot_base_interface_included

methodmap NavBotBotInterface
{
	/**
	 * Resets the interface to it's initial state.
	 */
	public native void Reset();

	property bool IsNull
	{
		public get() { return view_as<Address>(this) == Address_Null; }
	}
}