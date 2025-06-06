#ifndef NAVBOT_TF2_SENSOR_INTERFACE_H_
#define NAVBOT_TF2_SENSOR_INTERFACE_H_
#pragma once

#include <string>
#include <unordered_set>

#include <bot/interfaces/sensor.h>

class CTF2BotSensor : public ISensor
{
public:
	CTF2BotSensor(CBaseBot* bot);
	virtual ~CTF2BotSensor();

	bool IsIgnored(CBaseEntity* entity) override;
	bool IsFriendly(CBaseEntity* entity) override;
	bool IsEnemy(CBaseEntity* entity) override;
	int GetKnownEntityTeamIndex(CKnownEntity* known) override;
	void OnTruceChanged(const bool enabled) override;

private:
	std::unordered_set<std::string> m_classname_filter;

	bool IsPlayerIgnoredInternal(CBaseEntity* entity);
	bool IgnoredConditionsInternal(CBaseEntity* player);

	inline void AddClassnametoFilter(const char* classname)
	{
		m_classname_filter.emplace(classname);
	}

	bool IsClassnameIgnored(const char* classname);
};

#endif // !NAVBOT_TF2_SENSOR_INTERFACE_H_
