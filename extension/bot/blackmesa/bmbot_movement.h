#ifndef NAVBOT_BLACK_MESA_BOT_MOVEMENT_H_
#define NAVBOT_BLACK_MESA_BOT_MOVEMENT_H_

#include <bot/interfaces/movement.h>

class CBlackMesaBot;


class CBlackMesaBotMovement : public IMovement
{
public:
	CBlackMesaBotMovement(CBaseBot* bot);
	~CBlackMesaBotMovement() override;

};

#endif // !NAVBOT_BLACK_MESA_BOT_MOVEMENT_H_
