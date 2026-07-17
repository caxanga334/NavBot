#ifndef __NAVBOT_ZPS_MOD_H_
#define __NAVBOT_ZPS_MOD_H_

#include <mods/basemod.h>
#include "zps_lib.h"
#include "zps_objective_manager.h"

class CZombiePanicSourceMod : public CBaseMod
{
public:
	CZombiePanicSourceMod();

	static constexpr int MODEVENT_ZPS_NEW_OBJECTIVE = 1;

	static CZombiePanicSourceMod* GetZPSMod();

	void FireGameEvent(IGameEvent* event) override;

	void Update() override;

	CBaseBot* AllocateBot(edict_t* edict) override;
	CNavMesh* NavMeshFactory() override;
	void OnMapStart() override;
	void OnMapEnd() override;
	void OnRoundStart() override;
	void OnRoundEnd() override;
	bool IsTeamBasedGame() const override { return true; }
	IModHelpers* AllocModHelpers() const override;

	inline bool IsGameMode(zps::ZPSGamemodes mode) const { return m_gamemode == mode; }
	inline zps::ZPSGamemodes GetGameMode() const { return m_gamemode; }
	inline bool IsRoundActive() const { return m_roundactive; }
	inline const CZPSObjectiveManager& GetObjectiveManager() const { return m_objectiveManager; }

#ifndef NO_SOURCEPAWN_API
	void SMAPI_SetCurrentObjective(CZPSObjectiveManager::ObjectiveTypes type)
	{
		CZPSObjectiveManager::ObjectiveTypes old = m_objectiveManager.GetCurrentObjective();
		m_objectiveManager.SetCurrentObjective(type);

		if (old != type)
		{
			OnCurrentObjectiveChanged();
		}
	}
	void SMAPI_SetObjectiveMoveGoal(const Vector& goal) { m_objectiveManager.SetMoveGoal(goal); }
	void SMAPI_SetObjectiveUseButton(CBaseEntity* entity) { m_objectiveManager.SetUseButton(entity); }
	void SMAPI_SetObjectiveItemSearchID(const char* str) { m_objectiveManager.SetItemSearchID(str); }
	void SMAPI_SetObjectiveItemUseTarget(CBaseEntity* entity) { m_objectiveManager.SetUseItemTarget(entity); }
	void SMAPI_SetObjectiveDetectionRadius(float radius) { m_objectiveManager.SetDetectionRadius(radius); }
	void SMAPI_SetObjectiveGenericTargetEntity(CBaseEntity* entity) { m_objectiveManager.SetGenericTargetEntity(entity); }
	void SMAPI_ResetObjective() { m_objectiveManager.Reset(); }
#endif // !NO_SOURCEPAWN_API

protected:
	CWeaponInfoManager* CreateWeaponInfoManager() const override;

private:
	CountdownTimer m_roundstarttimer;
	zps::ZPSGamemodes m_gamemode;
	bool m_roundactive;
	CZPSObjectiveManager m_objectiveManager;

	void DetectGameMode();
	void OnCurrentObjectiveChanged();
};


#endif // !__NAVBOT_ZPS_MOD_H_
