/**
 * Optional stock functions for NavBot
 */
 
#if defined _navbot_stocks_included
 #endinput
#endif
#define _navbot_stocks_included

#include <sourcemod>
#include <sdktools>
#include <navbot>

enum struct NavAreaDrawData_s
{
	int client;
	int model;
	int halo;
	float width;
	float life;
	int colors[4]; // RGBA

	void SetModelData(int modelIndex, int haloIndex, float flwidth)
	{
		this.model = modelIndex;
		this.halo = haloIndex;
		this.width = flwidth;
	}

	void SendToAll()
	{
		this.client = -1;
	}

	void SendToAllInRange()
	{
		this.client = 0;
	}

	void SendToSpecificClient(int iclient)
	{
		this.client = iclient;
	}

	void SetColorRed()
	{
		this.colors[0] = 255;
		this.colors[1] = 0;
		this.colors[2] = 0;
		this.colors[3] = 255;
	}

	void SetColorGreen()
	{
		this.colors[0] = 0;
		this.colors[1] = 255;
		this.colors[2] = 0;
		this.colors[3] = 255;
	}

	void SetColorYellow()
	{
		this.colors[0] = 255;
		this.colors[1] = 255;
		this.colors[2] = 0;
		this.colors[3] = 255;
	}

	void SetColorBlue()
	{
		this.colors[0] = 0;
		this.colors[1] = 0;
		this.colors[2] = 255;
		this.colors[3] = 255;
	}

	void SetColorWhite()
	{
		this.colors[0] = 255;
		this.colors[1] = 255;
		this.colors[2] = 255;
		this.colors[3] = 255;
	}

	void SetColorMagenta()
	{
		this.colors[0] = 255;
		this.colors[1] = 0;
		this.colors[2] = 255;
		this.colors[3] = 255;
	}

	void SendTE(const float origin[3])
	{
		if (this.client < 0)
		{
			TE_SendToAll();
		}
		else if (this.client == 0)
		{
			TE_SendToAllInRange(origin, RangeType_Visibility);
		}
		else
		{
			TE_SendToClient(this.client);
		}
	}
}

/**
 * Draws a single nav area.
 * 
 * @param area          Address of a nav area.
 * @param data          Draw data.
 * @error               NULL nav area.
 */
stock void NB_DrawSingleNavArea(Address area, NavAreaDrawData_s data)
{
	if (area == Address_Null)
	{
		ThrowError("NULL nav area!");
	}

	float start[3];
	float end[3];
	float center[3];
	const int startFrame = 0;
	const int frameRate = 4;

	NavBotNavArea.GetCenter(area, center);
	NavBotNavArea.GetCorner(area, NAVBOT_NAV_CORNER_NORTH_WEST, start);
	NavBotNavArea.GetCorner(area, NAVBOT_NAV_CORNER_NORTH_EAST, end);

	TE_SetupBeamPoints(start, end, data.model, data.halo, startFrame, frameRate, data.life, data.width, data.width, 1, 0.0, data.colors, 0);
	data.SendTE(center);

	NavBotNavArea.GetCorner(area, NAVBOT_NAV_CORNER_SOUTH_WEST, end);

	TE_SetupBeamPoints(start, end, data.model, data.halo, startFrame, frameRate, data.life, data.width, data.width, 1, 0.0, data.colors, 0);
	data.SendTE(center);

	NavBotNavArea.GetCorner(area, NAVBOT_NAV_CORNER_SOUTH_EAST, start);

	TE_SetupBeamPoints(start, end, data.model, data.halo, startFrame, frameRate, data.life, data.width, data.width, 1, 0.0, data.colors, 0);
	data.SendTE(center);

	NavBotNavArea.GetCorner(area, NAVBOT_NAV_CORNER_NORTH_EAST, end);

	TE_SetupBeamPoints(start, end, data.model, data.halo, startFrame, frameRate, data.life, data.width, data.width, 1, 0.0, data.colors, 0);
	data.SendTE(center);
}

/**
 * Draws the nav area under the client's crosshair.
 * 
 * @param client			Client to draw to.
 * @param modelIndex		Index of a precached model to use.
 * @param haloIndex			Index of a precachhed model to use as halo.
 * @param life				How long to draw the areas.
 * @param drawAdjacentAreas	Optional, if true, also draws adjacent nav areas.
 * @error					Invalid client.
 */
stock void NB_DrawClientActiveNavArea(int client, const int modelIndex, const int haloIndex, const float life, const bool drawAdjacentAreas = true)
{
	if (client <= 0 || client > MaxClients)
	{
		ThrowError("Client %i is out of bounds!", client);
	}

	if (!IsClientInGame(client))
	{
		ThrowError("Client %i is not in game!", client);
	}

	if (IsFakeClient(client))
	{
		return;
	}

	Address area = NavBotNavMesh.FindActiveNavAreaForPlayer(client);

	if (area == Address_Null)
	{
		return;
	}

	NavAreaDrawData_s data;
	data.SendToSpecificClient(client);
	data.SetColorYellow();
	data.SetModelData(modelIndex, haloIndex, 1.0);
	data.life = life;

	NB_DrawSingleNavArea(area, data);

	if (drawAdjacentAreas)
	{
		data.SetColorRed();

		for (int dir = 0; dir < view_as<int>(NAVBOT_NAV_DIR_NUM_DIRECTIONS); dir++)
		{
			int limit = NavBotNavArea.GetAdjacentAreaCount(area, view_as<NavBotNavDirType>(dir));

			for (int i = 0; i < limit; i++)
			{
				Address other = NavBotNavArea.GetAdjacentArea(area, view_as<NavBotNavDirType>(dir), i);
				NB_DrawSingleNavArea(other, data);
			}
		}
	}
}

// Simple path cost callback for NavBotNavPathCostFunc.
stock float NB_SimplePathCost(Address toArea, Address fromArea, Address ladder, Address offmeshconn, Address elevator, float length)
{
	#pragma unused length

	if (fromArea == Address_Null)
	{
		// First area in the path, no cost.
		return 0.0;
	}

	float dist = 0.0;

	if (offmeshconn != Address_Null)
	{
		dist = NavBotNavOffMeshConnection.GetConnectionLength(offmeshconn);
	}
	else
	{
		// for now just use distance between the areas
		float v1[3];
		float v2[3];
		NavBotNavArea.GetCenter(toArea, v1);
		NavBotNavArea.GetCenter(fromArea, v2);
		dist = GetVectorDistance(v1, v2);
	}

	float costSoFar = NavBotNavArea.GetCostSoFar(fromArea);
	float cost = dist + costSoFar;
	return cost;
}

// Stock callback function for NavBotNavPathCostFunc. Only allow ground paths (no ladders and elevators)
stock float NB_SimpleGroundOnlyPathCost(Address toArea, Address fromArea, Address ladder, Address offmeshconn, Address elevator, float length)
{
	#pragma unused length

	if (fromArea == Address_Null)
	{
		// First area in the path, no cost.
		return 0.0;
	}

	float dist = 0.0;

	if (offmeshconn != Address_Null)
	{
		dist = NavBotNavOffMeshConnection.GetConnectionLength(offmeshconn);
	}
	else if (ladder != Address_Null || elevator != Address_Null)
	{
		return -1.0; // return -1 tells the path finder this is a dead end.
	}
	else
	{
		// for now just use distance between the areas
		float v1[3];
		float v2[3];
		NavBotNavArea.GetCenter(toArea, v1);
		NavBotNavArea.GetCenter(fromArea, v2);
		dist = GetVectorDistance(v1, v2);
	}

	float costSoFar = NavBotNavArea.GetCostSoFar(fromArea);
	float cost = dist + costSoFar;
	return cost;
}

// Simple g cost function callback for NavBotNavMesh.DoAStarSearch
stock float NB_SimpleGCost(Address toArea, Address fromArea, Address ladder, Address offmeshconn, Address elevator)
{
	if (fromArea == Address_Null)
	{
		// First area in the path, no cost.
		return 0.0;
	}

	float dist = 0.0;

	if (offmeshconn != Address_Null)
	{
		dist = NavBotNavOffMeshConnection.GetConnectionLength(offmeshconn);
	}
	else
	{
		// for now just use distance between the areas
		float v1[3];
		float v2[3];
		NavBotNavArea.GetCenter(toArea, v1);
		NavBotNavArea.GetCenter(fromArea, v2);
		dist = GetVectorDistance(v1, v2);
	}

	float cost = dist;
	return cost;
}

// Simple g cost function callback for NavBotNavMesh.DoAStarSearch. Only allows ground paths (no ladders and elevators)
stock float NB_SimpleGroundOnlyGCost(Address toArea, Address fromArea, Address ladder, Address offmeshconn, Address elevator)
{
	if (fromArea == Address_Null)
	{
		// First area in the path, no cost.
		return 0.0;
	}

	float dist = 0.0;

	if (offmeshconn != Address_Null)
	{
		dist = NavBotNavOffMeshConnection.GetConnectionLength(offmeshconn);
	}
	else if (ladder != Address_Null || elevator != Address_Null)
	{
		return -1.0; // return -1 tells the path finder this is a dead end.
	}
	else
	{
		// for now just use distance between the areas
		float v1[3];
		float v2[3];
		NavBotNavArea.GetCenter(toArea, v1);
		NavBotNavArea.GetCenter(fromArea, v2);
		dist = GetVectorDistance(v1, v2);
	}

	float cost = dist;
	return cost;
}

// Simple h cost function callback for NavBotNavMesh.DoAStarSearch
stock float NB_SimpleHCost(Address area, const float goal[3])
{
	float center[3];
	NavBotNavArea.GetCenter(area, center);
	return GetVectorDistance(center, goal);
}