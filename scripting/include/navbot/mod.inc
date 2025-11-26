#if defined _navbot_mod_interface_included
 #endinput
#endif
#define _navbot_mod_interface_included

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
}