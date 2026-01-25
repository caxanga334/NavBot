#ifndef __NAVBOT_SENSOR_INTERFACE_TEMPLATES_H_
#define __NAVBOT_SENSOR_INTERFACE_TEMPLATES_H_

#include "sensor.h"
#include <mods/modhelpers.h>

/**
 * @brief Simple sensor interface implementation.
 * @tparam Bot Bot class
 */
template <typename Bot>
class CSimpleSensor : public ISensor
{
public:
	CSimpleSensor(Bot* bot) : 
		ISensor(bot)
	{
	}

	bool IsIgnored(CBaseEntity* entity) const override
	{
		// ignore anything that doesn't derive from CBaseCombatCharacter
		return !modhelpers->IsCombatCharacter(entity);
	}

	bool IsFriendly(CBaseEntity* entity) const override
	{
		Bot* bot = GetBot<Bot>();
		return bot->GetCurrentTeamIndex() == modhelpers->GetEntityTeamNumber(entity);
	}

	bool IsEnemy(CBaseEntity* entity) const override
	{
		Bot* bot = GetBot<Bot>();
		return bot->GetCurrentTeamIndex() != modhelpers->GetEntityTeamNumber(entity);
	}
};

/**
 * @brief Simple sensor interface implementation for PvP only games.
 * @tparam Bot Bot class
 */
template <typename Bot>
class CSimplePvPSensor : public CSimpleSensor<Bot>
{
public:
	CSimplePvPSensor(Bot* bot) :
		CSimpleSensor<Bot>(bot)
	{
	}

	bool IsIgnored(CBaseEntity* entity) const override
	{
		return !modhelpers->IsPlayer(entity);
	}

protected:
	void CollectNonPlayerEntities(std::vector<edict_t*>& visibleVec) override {} /* Don't scan NPCs in PvP mods. */

private:

};

/**
 * @brief Simple sensor implementation with a entity classname filter.
 * @tparam Bot Bot class.
 */
template <typename Bot>
class CSimpleSensorFilter : public CSimpleSensor<Bot>
{
public:
	CSimpleSensorFilter(Bot* bot) :
		CSimpleSensor<Bot>(bot)
	{
	}

	bool IsIgnored(CBaseEntity* entity) const override
	{
		const char* classname = gamehelpers->GetEntityClassname(entity);

		if (!classname) { return true; }

		std::string strclassname{ classname };

		// ignored if not in the filter
		return m_classname_filter.find(strclassname) == m_classname_filter.end();
	}

protected:
	inline void AddEntityClassnameToFilter(const char* classname)
	{
		m_classname_filter.emplace(classname);
	}

private:
	std::unordered_set<std::string> m_classname_filter;
};

#endif // !__NAVBOT_SENSOR_INTERFACE_TEMPLATES_H_

