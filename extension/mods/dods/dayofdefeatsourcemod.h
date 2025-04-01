#ifndef SMNAV_DODS_MOD_H_
#define SMNAV_DODS_MOD_H_
#pragma once

#include <mods/basemod.h>
#include <sdkports/sdk_ehandle.h>

class CDODObjectiveResource
{
public:
	CDODObjectiveResource() = default;

	void Init(CBaseEntity* entity);

	// Number of control points on the map.
	int GetControlPointCount() const { return *m_iNumControlPoints; }
	bool IsControlPointVisible(int index) const { return m_bCPIsVisible[index]; }
	int GetAlliesRequiredCappers(int index) const { return m_iAlliesReqCappers[index]; }
	int GetAxisRequiredCappers(int index) const { return m_iAxisReqCappers[index]; }
	bool IsBombPlanted(int index) const { return m_bBombPlanted[index]; }
	int GetNumBombsRequired(int index) const { return m_iBombsRequired[index]; }
	int GetNumBombsRemaining(int index) const { return m_iBombsRemaining[index]; }
	bool IsBombBeingDefused(int index) const { return m_bBombBeingDefused[index]; }
	int GetNumAlliesOnPoint(int index) const { return m_iNumAllies[index]; }
	int GetNumAxisOnPoint(int index) const { return m_iNumAxis[index]; }
	int GetCapturingTeamIndex(int index) const { return m_iCappingTeam[index]; }
	int GetOwnerTeamIndex(int index) const { return m_iOwner[index]; }

private:
	int* m_iNumControlPoints; // Number of control points, this is not an array
	/* These are all arrays */
	bool* m_bCPIsVisible;
	int* m_iAlliesReqCappers;
	int* m_iAxisReqCappers;
	bool* m_bBombPlanted;
	int* m_iBombsRequired;
	int* m_iBombsRemaining;
	bool* m_bBombBeingDefused;
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

	void FireGameEvent(IGameEvent* event) override;

	const char* GetModName() override { return "Day of Defeat: Source"; }
	Mods::ModType GetModType() override { return Mods::ModType::MOD_DODS; }

	// Called by the manager when allocating a new bot instance
	CBaseBot* AllocateBot(edict_t* edict) override;

	void OnRoundStart() override;
	// NULL if the objective resource entity is invalid
	const CDODObjectiveResource* GetDODObjectiveResource() const;

private:
	CDODObjectiveResource m_objectiveres;
	CHandle<CBaseEntity> m_objectiveentity;

	void FindObjectiveResourceEntity();
};

#endif // !SMNAV_DODS_MOD_H_
