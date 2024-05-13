#ifndef SMNAV_TF_NAV_AREA_H_
#define SMNAV_TF_NAV_AREA_H_
#pragma once

#include <mods/tf2/teamfortress2_shareddefs.h>
#include <navmesh/nav_area.h>

class CTFNavArea : public CNavArea
{
public:
	inline CTFNavArea(unsigned int place) : CNavArea(place)
	{
		m_tfpathattributes = 0;
		m_tfattributes = 0;
		m_mvmattributes = 0;
		m_spawnroomteam = 0;
	}

	void Save(std::fstream& filestream, uint32_t version) override;
	NavErrorType Load(std::fstream& filestream, uint32_t version, uint32_t subVersion) override;
	void UpdateBlocked(bool force = false, int teamID = NAV_TEAM_ANY) override;
	bool IsBlocked(int teamID, bool ignoreNavBlockers = false) const override;

	// Pathing flags
	enum TFNavPathAttributes
	{
		TFNAV_PATH_INVALID = 0,
		TFNAV_PATH_NO_RED_TEAM = (1 << 0), // Red team can't use this area
		TFNAV_PATH_NO_BLU_TEAM = (1 << 1), // Blu team can't use this area
		TFNAV_PATH_NO_CARRIERS = (1 << 2), // Flag/Objective carriers can't use this area
		TFNAV_PATH_CARRIERS_AVOID = (1 << 3), // Flag/Objective carriers will avoid using this area
		TFNAV_PATH_DYNAMIC_SPAWNROOM = (1 << 4), // Dynamic No Red/Blu based on the nearest spawnroom entity
	};

	void SetTFPathAttributes(TFNavPathAttributes attribute)
	{
		m_tfpathattributes |= attribute;
	}

	void ClearTFPathAttributes(TFNavPathAttributes attribute)
	{
		m_tfpathattributes &= ~attribute;
	}

	bool HasTFPathAttributes(TFNavPathAttributes attributes) const
	{
		return (m_tfpathattributes & attributes) ? true : false;
	}

	bool CanBeUsedByTeam(TeamFortress2::TFTeam team) const
	{
		if (HasTFPathAttributes(TFNAV_PATH_NO_RED_TEAM) && team == TeamFortress2::TFTeam_Red)
		{
			return false;
		}

		if (HasTFPathAttributes(TFNAV_PATH_NO_BLU_TEAM) && team == TeamFortress2::TFTeam_Blue)
		{
			return false;
		}

		// no need to check for TFNAV_PATH_DYNAMIC_SPAWNROOM as m_spawnroomteam defaults to 0 without it
		if (m_spawnroomteam > 1 && m_spawnroomteam != static_cast<int>(team))
		{
			return false;
		}

		return true;
	}

	enum TFNavAttributes
	{
		TFNAV_INVALID = 0,
		TFNAV_LIMIT_TO_REDTEAM = (1 << 0), // Hints are limited to RED team only, this does not block pathing!
		TFNAV_LIMIT_TO_BLUTEAM = (1 << 1), // Hints are limited to BLU team only, this does not block pathing!
		TFNAV_SENTRYGUN_HINT = (1 << 2), // This is a good spot to build a sentry gun
		TFNAV_DISPENSER_HINT = (1 << 3), // This is a good spot to build a dispenser
		TFNAV_TELE_ENTRANCE_HINT = (1 << 4), // This is a good spot to build a teleporter entrance
		TFNAV_TELE_EXIT_HINT = (1 << 5), // This is a good spot to build a teleporter exit
		TFNAV_SNIPER_HINT = (1 << 6), // This is a good spot to snipe from
	};

	void SetTFAttributes(TFNavAttributes attribute)
	{
		m_tfattributes |= attribute;
	}

	void ClearTFAttributes(TFNavAttributes attribute)
	{
		m_tfattributes &= ~attribute;
	}

	bool HasTFAttributes(TFNavAttributes attributes) const
	{
		return (m_tfattributes & attributes) ? true : false;
	}

	enum MvMNavAttributes
	{
		MVMNAV_INVALID = 0,
		MVMNAV_FRONTLINES = (1 << 0), // Bots will move here while waiting for the wave to start
	};

	void SetMVMAttributes(MvMNavAttributes attribute)
	{
		m_mvmattributes |= attribute;
	}

	void ClearMVMAttributes(MvMNavAttributes attribute)
	{
		m_mvmattributes &= ~attribute;
	}

	bool HasMVMAttributes(MvMNavAttributes attributes) const
	{
		return (m_mvmattributes & attributes) ? true : false;
	}

	void UpdateDynamicSpawnRoom();

	void Debug_ShowTFPathAttributes() const;
	void Debug_ShowTFAttributes() const;
	void Debug_ShowMvMAttributes() const;

private:
	int m_tfpathattributes;
	int m_tfattributes;
	int m_mvmattributes; // Attributes exclusive for the Mann vs Machine game modes
	int m_spawnroomteam;
};

#endif // !SMNAV_TF_NAV_AREA_H_
