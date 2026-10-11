#if defined _navbot_navladder_included
 #endinput
#endif
#define _navbot_navladder_included

methodmap NavBotNavLadder
{
	/**
	 * Returns the ladder's length.
	 * 
	 * @param ptr			Address of a nav ladder.
	 * @return				Ladder's length.
     * @error               Bad ladder ptr.
	 */
	public static native float GetLength(Address ptr);
}