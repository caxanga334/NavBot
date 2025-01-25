#ifndef NAVBOT_BLACKMESA_BOT_H_
#define NAVBOT_BLACKMESA_BOT_H_

#include <memory>
#include <mods/blackmesa/blackmesa_shareddefs.h>
#include <bot/basebot.h>
#include <bot/interfaces/path/basepath.h>
#include "bmbot_movement.h"
#include "bmbot_sensor.h"
#include "bmbot_behavior.h"

class CBlackMesaBot : public CBaseBot
{
public:
	CBlackMesaBot(edict_t* edict);
	~CBlackMesaBot() override;

	bool HasJoinedGame() override;
	void TryJoinGame() override;

	void FirstSpawn() override;

	CBlackMesaBotMovement* GetMovementInterface() const override { return m_bmmovement.get(); }
	CBlackMesaBotSensor* GetSensorInterface() const override { return m_bmsensor.get(); }

	blackmesa::BMTeam GetMyBMTeam() const;

private:
	std::unique_ptr<CBlackMesaBotMovement> m_bmmovement;
	std::unique_ptr<CBlackMesaBotSensor> m_bmsensor;
};

#endif // !NAVBOT_BLACKMESA_BOT_H_
