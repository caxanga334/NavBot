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
	~CExtManager();

	// Called when all extensions have been loaded
	void OnAllLoaded();

	void Frame();

	void OnClientPutInServer(int client);

	void OnClientDisconnect(int client);

	void OnMapStart();
	void OnMapEnd();

	void AllocateMod();

	CBaseMod* GetMod();

	void NotifyRegisterGameEvents();

	CBaseBot* GetBotByIndex(int index);

	void AddBot();

	// Gets a vector of all bots currently in game
	const auto &GetAllBots() const { return m_bots; }

	void LoadBotNames();

private:
	std::vector<std::unique_ptr<CBaseBot>> m_bots; // Vector of bots
	std::vector<std::string> m_botnames; // Vector of names to be used by bots
	std::unique_ptr<CBaseMod> m_mod;
	size_t m_nextbotname; // Index of the next bot name to use
};

#endif // !SMNAV_EXT_MANAGER_H_

