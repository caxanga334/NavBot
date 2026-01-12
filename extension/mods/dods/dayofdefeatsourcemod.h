#ifndef __NAVBOT_DODS_MOD_H_
#define __NAVBOT_DODS_MOD_H_
#pragma once

#include <queue>
#include <array>
#include <vector>
#include <bot/dods/dodsbot_sharedmemory.h>
#include <mods/basemod.h>
#include <sdkports/sdk_ehandle.h>
#include "dods_shareddefs.h"

class CDoDSBot;
class CDoDSNavArea;
class CDoDSWaypoint;

class CDoDModSettings : public CModSettings
{
public:
	CDoDModSettings()
	{
		bomb_plant_respond_dist = 3000.0f;
		bomb_max_defusers = 3;
	}

	void SetMaxBombPlantedRespondDistance(float dist) { bomb_plant_respond_dist = dist; }
	void SetMaxBombDefusers(int num) { bomb_max_defusers = num; }

	const float GetMaxBombPlantedRespondDistance() const { return bomb_plant_respond_dist; }
	int GetMaxBombDefusers() const { return bomb_max_defusers; }

protected:
	SourceMod::SMCResult ReadSMC_KeyValue(const SourceMod::SMCStates* states, const char* key, const char* value) override;

private:
	float bomb_plant_respond_dist; // maximum distance for a bot to respond to a bomb plant event
	int bomb_max_defusers; // maximum number of bots trying to defuse the same bomb
};

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
	void Update() override;
	// Called by the manager when allocating a new bot instance
	CBaseBot* AllocateBot(edict_t* edict) override;
	CNavMesh* NavMeshFactory() override;
	void OnMapStart() override;
	void OnRoundStart() override;
	// NULL if the objective resource entity is invalid
	const CDODObjectiveResource* GetDODObjectiveResource() const;
	/**
	 * @brief Gets the control point data struct by index.
	 * @param index Index of the control point.
	 * @return Control point data struct or NULL if the control point index is invalid or doesn't exists.
	 */
	const CDayOfDefeatSourceMod::DoDControlPoint* GetControlPointByIndex(int index) const;
	/**
	 * @brief Given a control point entity, returns a bomb that needs to be defused.
	 * @param pControlPoint Control point entity.
	 * @return Bomb entity or NULL if no bomb is planted.
	 */
	CBaseEntity* GetControlPointBombToDefuse(CBaseEntity* pControlPoint) const;
	/**
	 * @brief Collects control points a team can attack.
	 * @param team Team which will be attacking the control points.
	 * @param points Vector to store the collected control points.
	 */
	void CollectControlPointsToAttack(dayofdefeatsource::DoDTeam team, std::vector<const CDayOfDefeatSourceMod::DoDControlPoint*>& points) const;
	/**
	 * @brief Collects control points a team can defend.
	 * @param team Team which will be defending the control points.
	 * @param points Vector to store the collected control points.
	 */
	void CollectControlPointsToDefend(dayofdefeatsource::DoDTeam team, std::vector<const CDayOfDefeatSourceMod::DoDControlPoint*>& points) const;
	/**
	 * @brief Returns if the current map uses bombs.
	 * @return True if yes, false if no.
	 */
	bool MapUsesBombs() const { return m_mapUsesBombs; }
	/**
	 * @brief Checks if the given control point index is valid.
	 * @param index Control point index to check.
	 * @return True if valid, false if invalid.
	 */
	bool IsValidControlPointIndex(const int index) const
	{
		return GetControlPointByIndex(index) != nullptr;
	}
	/**
	 * @brief Selects a random DoD class for the given bot.
	 * @param bot Bot that wants to change classes.
	 * @return Bot class.
	 */
	dayofdefeatsource::DoDClassType SelectClassForBot(CDoDSBot* bot) const;
	/**
	 * @brief Gets the class limit for the given class and team.
	 * @param cls Class to check.
	 * @param team Which team?
	 * @return Class limit.
	 */
	int GetClassLimit(dayofdefeatsource::DoDClassType cls, dayofdefeatsource::DoDTeam team) const;
	CDoDSSharedBotMemory* GetSharedBotMemory(int teamindex) override;
	const CDoDModSettings* GetDoDModSettings() const { return static_cast<const CDoDModSettings*>(GetModSettings()); }
protected:
	CModSettings* CreateModSettings() const override;
	ISharedBotMemory* CreateSharedMemory() const override;
	void RegisterModCommands() override;

private:
	CDODObjectiveResource m_objectiveres;
	CHandle<CBaseEntity> m_objectiveentity;
	std::array<DoDControlPoint, MAX_CONTROL_POINTS> m_controlpoints;
	bool m_mapUsesBombs;

	void FindObjectiveResourceEntity();
	void FindControlPoints();
	void FindAndAssignCaptureAreas();
	void FindAndAssignBombTargets();
	static constexpr auto BOMB_PLANTED = false;
	static constexpr auto BOMB_DEFUSED = true;

	struct BombEvent
	{
		BombEvent(IGameEvent* event, bool isDefused);

		bool isDefused;
		int userid;
		int cpindex;
		int team;
	};

	std::queue<BombEvent> m_bombevents;
	void OnBombEvent(const BombEvent& event) const;

	struct ClassLimits
	{
		ClassLimits()
		{
			rifleman = -1;
			assault = -1;
			support = -1;
			sniper = -1;
			mg = -1;
			rocket = -1;
		}

		int rifleman;
		int assault;
		int support;
		int sniper;
		int mg;
		int rocket;
	};

	ClassLimits m_classlimits_allies;
	ClassLimits m_classlimits_axis;
	bool m_random_class_allowed;

	void UpdateClassLimits();
};

#endif // !__NAVBOT_DODS_MOD_H_
