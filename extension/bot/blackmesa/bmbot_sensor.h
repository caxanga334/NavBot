#ifndef NAVBOT_BLACK_MESA_BOT_SENSOR_H_
#define NAVBOT_BLACK_MESA_BOT_SENSOR_H_

#include <bot/interfaces/sensor.h>

class CBlackMesaBot;


class CBlackMesaBotSensor: public ISensor
{
public:
	CBlackMesaBotSensor(CBaseBot* bot);
	~CBlackMesaBotSensor() override;

	bool IsIgnored(CBaseEntity* entity) override;
	bool IsFriendly(CBaseEntity* entity) override;
	bool IsEnemy(CBaseEntity* entity) override;

protected:
	void CollectNonPlayerEntities(std::vector<edict_t*>& visibleVec) override;
};

#endif // !NAVBOT_BLACK_MESA_BOT_SENSOR_H_
