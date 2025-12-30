#ifndef __NAVBOT_ZPS_MOD_H_
#define __NAVBOT_ZPS_MOD_H_

#include <mods/basemod.h>
#include "zps_lib.h"

class CZombiePanicSourceMod : public CBaseMod
{
public:
	CZombiePanicSourceMod();

	static CZombiePanicSourceMod* GetZPSMod();

	void FireGameEvent(IGameEvent* event) override;

	void Update() override;

	CBaseBot* AllocateBot(edict_t* edict) override;
	CNavMesh* NavMeshFactory() override;
	void OnMapStart() override;
	void OnRoundStart() override;
	void OnRoundEnd() override;

	bool IsEntityDamageable(CBaseEntity* entity, const int maxhealth = 1000) const override;
	bool IsEntityBreakable(CBaseEntity* entity) const override;

	inline bool IsGameMode(zps::ZPSGamemodes mode) const { return m_gamemode == mode; }
	inline zps::ZPSGamemodes GetGameMode() const { return m_gamemode; }
	inline bool IsRoundActive() const { return m_roundactive; }

private:
	CountdownTimer m_roundstarttimer;
	zps::ZPSGamemodes m_gamemode;
	bool m_roundactive;

	void DetectGameMode();
	void UpdateFootstepEvents();
};


#endif // !__NAVBOT_ZPS_MOD_H_
