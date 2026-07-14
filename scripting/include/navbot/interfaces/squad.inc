#if defined _navbot_interfaces_squad_included
 #endinput
#endif
#define _navbot_interfaces_squad_included

methodmap NavBotSquadInterface < NavBotBotInterface
{
	/**
	 * Checks if the bot in a squad.
	 * 
	 * @param ptr			Address to the squad interface.
	 * @return				True if the bot is in a valid squad, false otherwise.
	 */
	public static native bool IsInASquad(Address ptr);
	// Returns true if the bot can join squads.
	public static native bool CanJoinSquads(Address ptr);
	// Returns true if the bot can create and lead squads.
	public static native bool CanLeadSquads(Address ptr);
	// Returns true if the bot is the current squad leader.
	public static native bool IsSquadLeader(Address ptr);
	/**
	 * Checks if the bot's squad is led by a human.
	 * Name is a bit misleading, as the "human" leader may be another bot (that isn't a NavBot).
	 * 
	 * @param ptr			Address to the squad interface.
	 * @return				True if the squad is led by a human (See the comment above).
	 */
	public static native bool IsHumanLedSquad(Address ptr);
	/**
	 * Gets the entity index of the client leading the squad.
	 * The squad leader may be a NavBot bot, a human player or a non NavBot bot.
	 * 
	 * @param ptr			Address to the squad interface.
	 * @return				Entity index. Returns 0 if not in a squad or if the squad leader is NULL (client left the server).
	 */
	public static native int GetSquadLeader(Address ptr);
	/**
	 * Gets the entity index of the bot leader.
	 * Squads needs to be managed by a bot, for human led squads, this returns the bot managing the squad.
	 * 
	 * @param ptr			Address to the squad interface.
	 * @return				Entity index. Returns 0 if not in a squad.
	 */
	public static native int GetBotSquadLeader(Address ptr);
	// Returns the number of members in this squad.
	public static native int GetSquadMemberCount(Address ptr);
	/**
	 * Gets the entity index of a squad member.
	 * 
	 * @param ptr			Address to the squad interface.
	 * @param index			Index of the squad member. Range is between 0 and GetSquadMemberCount() - 1.
	 * @return				Entity index of the squad member, returns 0 if the member is NULL (client left the server).
	 */
	public static native int GetSquadMemberEntity(Address ptr, const int index);
	/**
	 * Creates a new squad. The bot the interface belongs to becomes the squad leader.
	 * 
	 * @param ptr			Address to the squad interface.
	 * @param client		Optional client entity index of a human player. If a valid entity passed, creates a human led squad.
	 * @return				True if the squad was created, false otherwise.
	 * @error				Bad interface address, NavBot instance passed in the 'client' param.
	 */
	public static native bool CreateSquad(Address ptr, const int client = INVALID_ENT_REFERENCE);
	/**
	 * Destroys the squad. If called on a squad leader bot, the squad is disbanded.
	 * If called on a member, the bot leaves the squad.
	 * 
	 * @param ptr			Address to the squad interface.
	 */
	public static native void DestroySquad(Address ptr);
	/**
	 * Adds a member to the current squad. Must be called on the squad leader.
	 * Humans can be added to bot squads. 
	 * 
	 * @param ptr			Address to the squad interface.
	 * @param client		Client index to add to the squad.
	 */
	public static native bool AddMemberToSquad(Address ptr, const int client);
	/**
	 * Removes a member to the current squad. Must be called on the squad leader.
	 * 
	 * @param ptr			Address to the squad interface.
	 * @param client		Client index to add to the squad.
	 */
	public static native void RemoveMemberFromSquad(Address ptr, const int client);
	/**
	 * Searches for a squad interface the given client is part of.
	 * This should be used to search for squad interfaces of non NavBot clients.
	 * 
	 * @param client		Index of the client to search a squad interface of.
	 * @param leaderonly	If true, only search for squads the client is a leader.
	 * @return				Squad interface pointer address. Returns Address_Null if not found.
	 * @error				NavBot instance passed to 'client' param.
	 */
	public static native Address GetHumanPlayerSquad(const int client, const bool leaderonly = false);
}
