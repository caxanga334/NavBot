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
	void CollectNonPlayerEntities(std::vector<CBaseEntity*>& visibleVec) override {} /* Don't scan NPCs in PvP mods. */

private:

};

#endif // !__NAVBOT_SENSOR_INTERFACE_TEMPLATES_H_

