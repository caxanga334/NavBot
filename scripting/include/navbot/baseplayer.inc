#if defined _navbot_baseplayer_included
 #endinput
#endif
#define _navbot_baseplayer_included

methodmap NavBotBasePlayer
{
	// Constructor
	// This does not mean the BasePlayer instance is valid. Use NavBotManager.GetBasePlayer and check with IsNull to guarantee a valid instance.
	public NavBotBasePlayer(int client)
	{
		return view_as<NavBotBasePlayer>(client);
	}
	// Gets the entity index of this player instance.
	property int Index
	{
		public get() { return view_as<int>(this); }
	}
	// Returns true if this player instance is NULL.
	property bool IsNull
	{
		public get() { return this.Index == 0; }
	}
	// Returns true if this player instance is a NavBot bot.
	property bool IsNavBot
	{
		public native get();
	}
	// Gets/Sets the player's team.
	property int TeamNum
	{
		public get() { return GetClientTeam(this.Index); }
		public set(int Team) { ChangeClientTeam(this.Index, Team); }
	}
	property bool IsAlive
	{
		public get() { return IsPlayerAlive(this.Index); }
	}
	/**
	 * Gets the player's last known nav area.
	 * This returns an address to a nav area from NavBot's nav mesh.
	 */
	public native Address GetLastKnownNavArea();
	// Forces an update to the last known nav area.
	public native void ForceUpdateLastKnownNavArea();
	/**
	 * Gets the player's local eye angles.
	 * 
	 * @param angles        Array to store the player's local eye angles.
	 * @error               Invalid BasePlayer instance.
	 */
	public native void GetLocalEyeAngles(float angles[3]);
	/**
	 * Gets the player's world space center.
	 * 
	 * @param angles        Array to store the player's world space center coordinates.
	 * @error               Invalid BasePlayer instance.
	 */
	public native void WorldSpaceCenter(float center[3]);
	/**
	 * Sets the player's eye angles.
	 * 
	 * @param angles        Angle to set.
	 * @error               Invalid BasePlayer instance.
	 */
	public native void SnapEyeAngles(const float angles[3]);
	/**
	 * Gets the player's absolute origin.
	 * 
	 * @param origin		Float array to write the player's absolute origin position.
	 */
	public void GetAbsOrigin(float origin[3])
	{
		GetClientAbsOrigin(this.Index, origin);
	}
	/**
	 * Gets the player's eye angles.
	 * 
	 * @param angles		Float array to write the player's eye angles.
	 */
	public void GetEyeAngles(float angles[3])
	{
		GetClientEyeAngles(this.Index, angles);
	}
	/**
	 * Gets the player's eye position.
	 * 
	 * @param eyepos		Float array to write the player's eye position.
	 */
	public void GetEyePosition(float eyepos[3])
	{
		GetClientEyePosition(this.Index, eyepos);
	}
}
