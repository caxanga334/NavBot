#ifndef SMNAV_EXT_MANAGER_H_
#define SMNAV_EXT_MANAGER_H_
#pragma once

class CBaseMod;
class CBaseExtPlayer;
class CBaseBot;

// Max players, TO-DO: Add per SDK
constexpr auto MAX_SUPPORTED_PLAYERS = 130;

// Primary Extension Manager
class CExtManager
{
public:
	CExtManager();
	virtual ~CExtManager();

	// Called when all extensions have been loaded
	virtual void OnAllLoaded();

	virtual void Frame();

	virtual void OnClientPutInServer(int client);

	virtual void OnClientDisconnect(int client);

	virtual void AllocateMod();

	virtual CBaseMod* GetMod();

	CBaseExtPlayer* GetPlayerByIndex(int index) { return m_players[index]; }

	CBaseBot* GetBotByIndex(int index);

private:
	// Array to store players and bots
	CBaseExtPlayer* m_players[MAX_SUPPORTED_PLAYERS];
};

#endif // !SMNAV_EXT_MANAGER_H_

