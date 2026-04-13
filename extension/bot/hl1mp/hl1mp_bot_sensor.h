#ifndef __NAVBOT_HL1MP_BOT_SENSOR_INTERFACE_H_
#define __NAVBOT_HL1MP_BOT_SENSOR_INTERFACE_H_

#include <bot/interfaces/sensor_templates.h>

class CHL1MPBot;

class CHL1MPBotSensor : public CSimplePvPSensor<CHL1MPBot>
{
public:
	CHL1MPBotSensor(CHL1MPBot* bot) :
		CSimplePvPSensor<CHL1MPBot>(bot)
	{
	}

	/* teamplay is basically broken on HL1:DMS, so it's always deathmatch */
	bool IsFriendly(CBaseEntity* entity) const override { return false; }
	bool IsEnemy(CBaseEntity* entity) const override { return true; }

protected:
	void CollectPlayers(std::vector<edict_t*>& visibleVec) override;
	// no-op on HL1:DMS since it's deathmatch only
	void UpdateSharedKnowns() override {}

private:

};


#endif // !__NAVBOT_HL1MP_BOT_SENSOR_INTERFACE_H_
