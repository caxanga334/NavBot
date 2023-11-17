#ifndef SMNAV_EXT_MANAGER_H_
#define SMNAV_EXT_MANAGER_H_
#pragma once

class CBaseMod;
class CBaseExtPlayer;
class CBaseBot;

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

	CBaseBot* GetBotByIndex(int index);

private:
	std::vector<CBaseBot*> m_bots; // Vector of bots
};

#endif // !SMNAV_EXT_MANAGER_H_

