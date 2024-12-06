#include <sourcemod>
#include <sdktools>
#include <sdkhooks>
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

bool g_isnavbot[MAXPLAYERS + 1];
MeshNavigator g_nav[MAXPLAYERS + 1];
float g_timeout[MAXPLAYERS + 1];
float g_repath[MAXPLAYERS + 1];
float g_dest[MAXPLAYERS + 1][3];

public void OnPluginStart()
{
	RegAdminCmd("sm_list_navbots", ConCmd_ListNavBots, ADMFLAG_GENERIC, "List all navbots in the server.");
	RegAdminCmd("sm_attach_navbot", ConCmd_BotMe, ADMFLAG_GENERIC, "Attaches a NavBot CPluginBot interface on you.");
	RegAdminCmd("sm_path_to_pos", ConCmd_PathToPosition, ADMFLAG_GENERIC, "Uses the NavBot Navigator to path to a position.");
}

public void OnMapStart()
{
	for (int i = 0; i < MAXPLAYERS + 1; i++)
	{
		g_isnavbot[i] = false;
		g_nav[i] = view_as<MeshNavigator>(-1);
		g_timeout[i] = 0.0;
	}
}

public void OnMapEnd()
{
	for (int i = 0; i < MAXPLAYERS + 1; i++)
	{
		g_isnavbot[i] = false;
		g_nav[i] = view_as<MeshNavigator>(-1);
		g_timeout[i] = 0.0;
	}
}

public void OnClientDisconnect(int client)
{
		g_isnavbot[client] = false;
		g_timeout[client] = 0.0;

		if (!g_nav[client].IsNull)
		{
			g_nav[client].Destroy();
			g_nav[client] = view_as<MeshNavigator>(-1);
		}
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

Action ConCmd_BotMe(int client, int args)
{
	if (!client)
	{
		return Plugin_Handled;
	}

	if (!IsNavMeshLoaded())
	{
		ReplyToCommand(client, "Map doesn't have a nav mesh!");
		return Plugin_Handled;
	}

	PluginBot navbot = PluginBot(client);

	if (!navbot.IsNull)
	{
		ReplyToCommand(client, "Attached CPluginBot instance to you!");
		g_isnavbot[client] = true;
	}

	return Plugin_Handled;
}

Action ConCmd_PathToPosition(int client, int args)
{
	if (!client)
	{
		return Plugin_Handled;
	}

	if (!IsNavMeshLoaded())
	{
		ReplyToCommand(client, "Map doesn't have a nav mesh!");
		return Plugin_Handled;
	}

	if (!g_isnavbot[client])
	{
		ReplyToCommand(client, "You don't have a navbot interface attached!");
		return Plugin_Handled;
	}

	if (args < 3)
	{
		ReplyToCommand(client, "Usage: sm_path_to_pos <x> <y> <z>");
		return Plugin_Handled;
	}

	if (g_nav[client].IsNull)
	{
		g_nav[client] = MeshNavigator();
		PrintToServer("g_nav[%i] = %i", client, g_nav[client].Index);
	}

	float pos[3];

	pos[0] = GetCmdArgFloat(1);
	pos[1] = GetCmdArgFloat(2);
	pos[2] = GetCmdArgFloat(3);

	NavBot bot = GetNavBotByIndex(client);

	g_nav[client].ComputeToPos(bot, pos, 0.0, false);
	g_timeout[client] = GetGameTime() + 30.0;
	g_repath[client] = GetGameTime() + 0.5;
	g_dest[client][0] = pos[0];
	g_dest[client][1] = pos[1];
	g_dest[client][2] = pos[2];

	PrintToChatAll("%N is moving to %f %f %f", client, pos[0], pos[1], pos[2]);

	return Plugin_Handled;
}

// from https://github.com/EfeDursun125/TF2-EBOTS
void TF2_MoveTo(int client, float flGoal[3], float fVel[3], float fAng[3])
{
	float flPos[3];
	GetClientAbsOrigin(client, flPos);
	
	float newmove[3];
	SubtractVectors(flGoal, flPos, newmove);
	
	newmove[1] = -newmove[1];
	
	float sin = Sine(fAng[1] * FLOAT_PI / 180.0);
	float cos = Cosine(fAng[1] * FLOAT_PI / 180.0);
	
	fVel[0] = cos * newmove[0] - sin * newmove[1];
	fVel[1] = sin * newmove[0] + cos * newmove[1];
	
	NormalizeVector(fVel, fVel);
	
	if(GetVectorDistance(flPos, flGoal, true) > 20.0)
	{
		ScaleVector(fVel, 450.0);
	}
}

public Action OnPlayerRunCmd(int client, int &buttons, int &impulse, float vel[3], float angles[3], int &weapon, int &subtype, int &cmdnum, int &tickcount, int &seed, int mouse[2])
{

	if (g_timeout[client] >= GetGameTime())
	{
		float moveTo[3];
		NavBot bot = GetNavBotByIndex(client);

		if (g_repath[client] < GetGameTime())
		{
			g_repath[client] = GetGameTime() + 0.5;
			g_nav[client].ComputeToPos(bot, g_dest[client], 0.0, false);
		}

		g_nav[client].Update(bot);
		g_nav[client].GetMoveGoal(moveTo);
		TF2_MoveTo(client, moveTo, vel, angles);

		float pos[3];
		GetClientAbsOrigin(client, pos);

		if (GetVectorDistance(pos, g_dest[client]) <= 48.0)
		{
			g_timeout[client] = 0.0;
			PrintToChatAll("%N reached it's destination!", client);
		}

		return Plugin_Changed;
	}


	return Plugin_Continue;
}