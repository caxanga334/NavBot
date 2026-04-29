#ifndef __NAVBOT_CSS_BOT_MOVEMENT_INTERFACE_H_
#define __NAVBOT_CSS_BOT_MOVEMENT_INTERFACE_H_

#include "../interfaces/movement.h"

class CCSSBot;

class CCSSBotMovement : public IMovement
{
public:
	CCSSBotMovement(CCSSBot* bot);

private:

};

#endif // !__NAVBOT_CSS_BOT_MOVEMENT_INTERFACE_H_
