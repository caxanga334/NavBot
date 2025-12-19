#ifndef __NAVBOT_DODS_MOD_H_
#define __NAVBOT_DODS_MOD_H_
#pragma once

#include <array>
#include <vector>
#include <mods/basemod.h>
#include <sdkports/sdk_ehandle.h>
#include "dods_shareddefs.h"

class CDODObjectiveResource
{
public:
	CDODObjectiveResource() = default;

	void Init(CBaseEntity* entity);

	// Number of control points on the map.
	int GetControlPointCount() const { return *m_iNumControlPoints; }
	bool IsControlPointVisible(int index) const { return m_bCPIsVisible[index] != 0; }
	int GetAlliesRequiredCappers(int index) const { return m_iAlliesReqCappers[index]; }
	int GetAxisRequiredCappers(int index) const { return m_iAxisReqCappers[index]; }
	bool IsBombPlanted(int index) const { return m_bBombPlanted[index] != 0; }
	int GetNumBombsRequired(int index) const { return m_iBombsRequired[index]; }
	int GetNumBombsRemaining(int index) const { return m_iBombsRemaining[index]; }
	bool IsBombBeingDefused(int index) const { return m_bBombBeingDefused[index] != 0; }
	int GetNumAlliesOnPoint(int index) const { return m_iNumAllies[index]; }
	int GetNumAxisOnPoint(int index) const { return m_iNumAxis[index]; }
	int GetCapturingTeamIndex(int index) const { return m_iCappingTeam[index]; }
	int GetOwnerTeamIndex(int index) const { return m_iOwner[index]; }

private:
	int* m_iNumControlPoints; // Number of control points, this is not an array
	/* These are all arrays */
	int* m_bCPIsVisible;
	int* m_iAlliesReqCappers;
	int* m_iAxisReqCappers;
	int* m_bBombPlanted;
	int* m_iBombsRequired;
	int* m_iBombsRemaining;
	int* m_bBombBeingDefused;
	int* m_iNumAllies;
	int* m_iNumAxis;
	int* m_iCappingTeam;
	int* m_iOwner;
};

class CDayOfDefeatSourceMod : public CBaseMod
{
public:
	CDayOfDefeatSourceMod();
	~CDayOfDefeatSourceMod() override;

	static CDayOfDefeatSourceMod* GetDODMod();

	struct DoDControlPoint
	{
		DoDControlPoint()
		{
			point.Term();
			capture_trigger.Term();

			for (auto& handle : bomb_targets)
			{
				handle.Term();
			}

			index = dayofdefeatsource::INVALID_CONTROL_POINT;
		}

		CHandle<CBaseEntity> point; // handle to dod_control_point
		CHandle<CBaseEntity> capture_trigger; // handle to dod_capture_area
		std::array<CHandle<CBaseEntity>, 16U> bomb_targets; // bomb targets assigned to this control point
		int index; // control point index

		inline void Reset()
		{
			point.Term();
			capture_trigger.Term();

			for (auto& handle : bomb_targets)
			{
				handle.Term();
			}

			index = dayofdefeatsource::INVALID_CONTROL_POINT;
		}
	};

	void FireGameEvent(IGameEvent* event) override;

	const char* GetModName() override { return "Day of Defeat: Source"; }
	Mods::ModType GetModType() override { return Mods::ModType::MOD_DODS; }

	// Called by the manager when allocating a new bot instance
	CBaseBot* AllocateBot(edict_t* edict) override;
	CNavMesh* NavMeshFactory() override;
	void OnMapStart() override;
	void OnRoundStart() override;
	// NULL if the objective resource entity is invalid
	const CDODObjectiveResource* GetDODObjectiveResource() const;
	const CDayOfDefeatSourceMod::DoDControlPoint* GetControlPointByIndex(int index) const;
	void CollectControlPointsToAttack(dayofdefeatsource::DoDTeam team, std::vector<const CDayOfDefeatSourceMod::DoDControlPoint*>& points) const;
	void CollectControlPointsToDefend(dayofdefeatsource::DoDTeam team, std::vector<const CDayOfDefeatSourceMod::DoDControlPoint*>& points) const;
	bool MapUsesBombs() const { return m_mapUsesBombs; }
private:
	CDODObjectiveResource m_objectiveres;
	CHandle<CBaseEntity> m_objectiveentity;
	std::array<DoDControlPoint, MAX_CONTROL_POINTS> m_controlpoints;
	bool m_mapUsesBombs;

	void FindObjectiveResourceEntity();
	void FindControlPoints();
	void FindAndAssignCaptureAreas();
	void FindAndAssignBombTargets();
};

#endif // !__NAVBOT_DODS_MOD_H_
