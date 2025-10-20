#ifndef __NAVBOT_ZPSBOT_SENSOR_H_
#define __NAVBOT_ZPSBOT_SENSOR_H_

#include <bot/interfaces/sensor.h>

class CZPSBot;

class CZPSBotSensor : public ISensor
{
public:
	CZPSBotSensor(CZPSBot* bot);
	~CZPSBotSensor() override;

	bool IsIgnored(CBaseEntity* entity) const override;
	bool IsFriendly(CBaseEntity* entity) const override;
	bool IsEnemy(CBaseEntity* entity) const override;

protected:
	void CollectNonPlayerEntities(std::vector<edict_t*>& visibleVec) override;

private:

};


#endif // !__NAVBOT_ZPSBOT_SENSOR_H_
