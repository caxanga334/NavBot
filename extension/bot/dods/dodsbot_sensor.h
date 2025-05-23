#ifndef __NAVBOT_DODSBOT_SENSOR_INTERFACE_H_
#define __NAVBOT_DODSBOT_SENSOR_INTERFACE_H_

#include <bot/interfaces/sensor.h>

class CDoDSBotSensor : public ISensor
{
public:
	CDoDSBotSensor(CBaseBot* bot);
	~CDoDSBotSensor() override;

	bool IsIgnored(CBaseEntity* entity) override;
	bool IsFriendly(CBaseEntity* entity) override;
	bool IsEnemy(CBaseEntity* entity) override;

	bool IsEntityHidden(CBaseEntity* entity) override;
	bool IsPositionObscured(const Vector& pos) override;

private:
	void CollectNonPlayerEntities(std::vector<edict_t*>& visibleVec) override;
};

#endif // !__NAVBOT_DODSBOT_SENSOR_INTERFACE_H_
