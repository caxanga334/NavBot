#if defined _navbot_mod_interface_included
 #endinput
#endif
#define _navbot_mod_interface_included

enum NavBotMapNameType
{
	MAPNAME_RAW = 0, // Raw map name without any modifications.
	MAPNAME_CLEAN, // Clean map name without workshop stuff
	MAPNAME_UNIQUE, // Clean and unique map name with a workshop ID
};

methodmap NavBotModInterface
{
	/**
	 * Returns the name of the mod currently detected by NavBot.
	 * 
	 * @param name			Buffer to store the mod name.
	 * @param size			Size of the name buffer.
	 */
	public static native void GetModName(char[] name, int size);
	/**
	 * Parses a specific mod settings file.
	 * 
	 * @param file			Absolute path to the file to be parsed.
	 * @return				True if the file was parsed without errors, false otherwise.
	 * @error				File doesn't exists.
	 */
	public static native bool ParseModSettingsFile(const char[] file);
	/**
	 * Parses a specific bot difficulty profiles file
	 * 
	 * @param file			Absolute path to the file to be parsed.
	 * @param replace		If true, clear existing difficulty profiles before parsing the file.
	 * @param refresh		If true, bots already in-game will refresh their difficulty profile.
	 * @return				True if the file was parsed without errors, false otherwise.
	 * @error				File doesn't exists.
	 */
	public static native bool ParseDifficultyProfileFile(const char[] file, const bool replace, const bool refresh);
	/**
	 * Parses a custom weapon config file. This will remove all existing entries.
	 * 
	 * @param file			Absolute path to the file to be parsed.
	 * @return				True if the file was parsed without errors, false otherwise.
	 * @error				File doesn't exists.
	 */
	public static native bool ParseWeaponConfigFile(const char[] file);
	/**
	 * Gets the current map name used by NavBot.
	 * 
	 * @param type			Map name type to get.
	 * @param buffer		Buffer to store the map name.
	 * @param size			Size of the buffer.
	 * @error				Invalid map type passed.
	 */
	public static native void GetCurrentMapName(NavBotMapNameType type, char[] buffer, int size);
	/**
	 * The name of the mod folder. This is the folder NavBot uses to load files from.
	 * 
	 * @param buffer		Buffer to store the mod folder.
	 * @param size			Size of the buffer.
	 */
	public static native void GetModFolder(char[] buffer, int size);
	/**
	 * The name of the fallback mod folder. This is a fallback folder in case a file doesn't exists in the main mod folder.
	 * 
	 * @param buffer		Buffer to store the mod folder.
	 * @param size			Size of the buffer.
	 * @return				True if a fallback folder is set. False if the current mod doesn't have a fallback folder.
	 */
	public static native bool GetFallbackModFolder(char[] buffer, int size);
	/**
	 * Adds a classname to the list of classnames the sensor interface will scan.
	 * Bots will test these entities for visibility and attack them if they're considered an enemy.
	 * 
	 * @param classname		Entity classname to add to the scan list.
	 */
	public static native void AddClassnameToNPCScanList(const char[] classname);
	// Removes an entity classname from the NPC scan list.
	public static native void RemoveClassnameFromNPCScanList(const char[] classname)
}