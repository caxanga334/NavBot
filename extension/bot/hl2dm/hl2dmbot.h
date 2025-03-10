#ifndef NAVBOT_HL2DM_BOT_H_
#define NAVBOT_HL2DM_BOT_H_

#include <bot/basebot.h>

class CHL2DMBot : public CBaseBot
{
public:
	CHL2DMBot(edict_t* entity);
	~CHL2DMBot() override;
	
	float GetMaxSpeed() const override { return 390.0f; } // sprint speed, gamemovement should cap the speed for us.
private:

};

#endif // !NAVBOT_HL2DM_BOT_H_
