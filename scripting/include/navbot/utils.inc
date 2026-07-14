#if defined _navbot_utils_included
 #endinput
#endif
#define _navbot_utils_included

// General purpose utility functions
methodmap NavBotUtils
{
	/**
	 * Gets an entity's world space center.
	 * 
	 * @param entity			Index or reference to an entity.
	 * @param center			Float array to store the world space center.
	 */
	public static native void GetEntityWorldSpaceCenter(const int entity, float center[3]);
}