#ifndef __NAVBOT_ZPSBOT_MOVEMENT_H_
#define __NAVBOT_ZPSBOT_MOVEMENT_H_

#include <bot/interfaces/movement.h>

class CZPSBot;

class CZPSBotMovement : public IMovement
{
public:
	CZPSBotMovement(CZPSBot* bot);

private:

};

#endif // !__NAVBOT_ZPSBOT_MOVEMENT_H_
