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

	virtual bool IsIgnored(edict_t* entity) override;
	virtual bool IsFriendly(edict_t* entity) override;
	virtual bool IsEnemy(edict_t* entity) override;
	virtual int GetKnownEntityTeamIndex(CKnownEntity* known) override;

private:
	std::unordered_set<std::string> m_classname_filter;

	inline void AddClassnametoFilter(const char* classname)
	{
		m_classname_filter.emplace(classname);
	}

	inline bool IsClassnameIgnored(const char* classname)
	{
		std::string szname(classname);
		// classname filter contains a list of classnames the bot cares about, if not found on the list, ignore it
		return m_classname_filter.find(szname) == m_classname_filter.end();
	}
};

#endif // !NAVBOT_TF2_SENSOR_INTERFACE_H_
