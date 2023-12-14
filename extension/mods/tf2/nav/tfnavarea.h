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
		m_spawnroomteam = 0;
	}

	virtual void Save(CUtlBuffer& fileBuffer, unsigned int version, unsigned int portversion) const override;
	virtual NavErrorType Load(CUtlBuffer& fileBuffer, unsigned int version, unsigned int portversion, unsigned int subVersion) override;

	// Pathing flags
	enum TFNavPathAttributes
	{
		TFNAV_PATH_NO_RED_TEAM = (1 << 0), // Red team can't use this area
		TFNAV_PATH_NO_BLU_TEAM = (1 << 1), // Blu team can't use this area
		TFNAV_PATH_NO_CARRIERS = (1 << 2), // Flag/Objective carriers can't use this area
		TFNAV_PATH_CARRIERS_AVOID = (1 << 3), // Flag/Objective carriers will avoid using this area
		TFNAV_PATH_DYNAMIC_SPAWNROOM = (1 << 4), // Dynamic No Red/Blu based on the nearest spawnroom entity
	};

	inline void SetTFPathAttributes(TFNavPathAttributes attribute)
	{
		m_tfpathattributes |= attribute;
	}

	inline void ClearTFPathAttributes(TFNavPathAttributes attribute)
	{
		m_tfpathattributes &= ~attribute;
	}

	inline bool HasTFPathAttributes(TFNavPathAttributes attributes) const
	{
		return (m_tfpathattributes & attributes) ? true : false;
	}

	inline bool CanBeUsedByTeam(TeamFortress2::TFTeam team) const
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

	void UpdateDynamicSpawnRoom();

private:
	int m_tfpathattributes;
	int m_tfattributes;
	int m_spawnroomteam;
};

#endif // !SMNAV_TF_NAV_AREA_H_
