#include <sourcemod>
#include <navbot>

#pragma newdecls required
#pragma semicolon 1

#define PLUGIN_VERSION "1.0.0"

public Plugin myinfo =
{
	name = "NavBot Admin Plugin",
	author = "caxanga334",
	description = "Allow server admins to manage NavBots.",
	version = PLUGIN_VERSION,
	url = "https://github.com/caxanga334/NavBot"
};


public void OnPluginStart()
{
	RegAdminCmd("sm_navbotadmin_addbot", CMD_AddBot, ADMFLAG_CONFIG, "Adds a NavBot bot to the game.");
}

public void OnLibraryRemoved(const char[] name)
{
	if (StrEqual(name, "navbot", false))
	{
		SetFailState("NavBot extension was unloaded!");
	}
}

Action CMD_AddBot(int client, int args)
{
	if (!IsNavMeshLoaded())
	{
		ReplyToCommand(client, "This map does not have a nav mesh loaded! Can't add bots.");
		return Plugin_Handled;
	}

	char botname[MAX_NAME_LENGTH];
	bool custom_name = false;
	bool result = false;
	int bot_index;

	if (args >= 2)
	{
		GetCmdArg(1, botname, sizeof(botname));

		if (strlen(botname) >= 3)
		{
			custom_name = true;
		}
	}

	if (custom_name)
	{
		result = AddNavBot(bot_index, botname);
	}
	else
	{
		result = AddNavBot(bot_index);
	}

	if (!result)
	{
		ReplyToCommand(client, "Failed to add a NavBot!");
		return Plugin_Handled;
	}

	ShowActivity2(client, "[SM] ", "Added a NavBot to the game.");
	LogAction(client, -1, "%L added a NavBot to the game.", client);

	return Plugin_Handled;
}