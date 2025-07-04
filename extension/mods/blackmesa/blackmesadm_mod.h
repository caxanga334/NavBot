#ifndef NAVBOT_BLACKMESA_DEATHMATCH_MOD_H_
#define NAVBOT_BLACKMESA_DEATHMATCH_MOD_H_

#include <mods/basemod.h>
#include "blackmesa_shareddefs.h"

class CBlackMesaDeathmatchMod : public CBaseMod
{
public:
	CBlackMesaDeathmatchMod();
	~CBlackMesaDeathmatchMod() override;

	static CBlackMesaDeathmatchMod* GetBMMod();

	void FireGameEvent(IGameEvent* event) override;
	void PostCreation() override;
	// Mod name (IE: Team Fortress 2)
	const char* GetModName() override { return "Black Mesa Deathmatch"; }
	// Mod ID
	Mods::ModType GetModType() override { return Mods::ModType::MOD_BLACKMESA; }
	CBaseBot* AllocateBot(edict_t* edict) override;
	CNavMesh* NavMeshFactory() override;
	void OnMapStart() override;
	void OnRoundStart() override;
	bool IsTeamPlay() const { return m_isTeamPlay; }
	int GetMaxCarryForAmmoType(blackmesa::BMAmmoIndex index) const { return m_maxCarry[static_cast<int>(index)]; }
	const std::pair<std::string, int>* GetRandomPlayerModel() const;

private:
	bool m_isTeamPlay;
	std::array<int, MAX_AMMO_TYPES> m_maxCarry;
	std::vector<std::pair<std::string, int>> m_playermodels; // pair of model name and skin count

	void BuildMaxCarryArray();
	void ParsePlayerModelsConfigFile();
};

#endif // !NAVBOT_BLACKMESA_DEATHMATCH_MOD_H_
