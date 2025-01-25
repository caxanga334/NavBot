#ifndef NAVBOT_BLACKMESA_DEATHMATCH_MOD_H_
#define NAVBOT_BLACKMESA_DEATHMATCH_MOD_H_

#include <mods/basemod.h>

class CBlackMesaDeathmatchMod : public CBaseMod
{
public:
	CBlackMesaDeathmatchMod();
	~CBlackMesaDeathmatchMod() override;

	static CBlackMesaDeathmatchMod* GetBMMod();

	void FireGameEvent(IGameEvent* event) override;

	// Mod name (IE: Team Fortress 2)
	const char* GetModName() override { return "Black Mesa Deathmatch"; }
	// Mod ID
	Mods::ModType GetModType() override { return Mods::ModType::MOD_BLACKMESA; }
	CBaseBot* AllocateBot(edict_t* edict) override;
	CNavMesh* NavMeshFactory() override;
	void OnMapStart() override;
	void OnRoundStart() override;
	bool IsTeamPlay() const { return m_isTeamPlay; }

private:
	bool m_isTeamPlay;
};

#endif // !NAVBOT_BLACKMESA_DEATHMATCH_MOD_H_
