#include <sourcemod>
#include <navbot>

#pragma newdecls required
#pragma semicolon 1


public Plugin myinfo =
{
    name = "NavBot Example Plugin",
    author = "caxanga334",
    description = "Plugin to demonstrate NavBot natives.",
    version = "1.0.0",
    url = "https://github.com/caxanga334/NavBot/"
};


public void OnPluginStart()
{
    RegAdminCmd("sm_list_navbots", ConCmd_ListNavBots, ADMFLAG_GENERIC, "List all navbots in the server.");
}

Action ConCmd_ListNavBots(int client, int args)
{
    for(int i = 1; i <= MaxClients; i++)
    {
        if (IsClientInGame(i))
        {
            if (IsFakeClient(i))
            {
                if (IsNavBot(i))
                {
                    PrintToChatAll("Client %N is a NavBot.", i);
                }
                else
                {
                    PrintToChatAll("Client %N is a Fake Client.", i);
                }
            }
            else
            {
                PrintToChatAll("Client %N is a Human.", i);
            }
        }
    }

    return Plugin_Handled;
}