#ifndef __NAVBOT_CSS_BOT_SENSOR_INTERFACE_H_
#define __NAVBOT_CSS_BOT_SENSOR_INTERFACE_H_

#include "../interfaces/sensor_templates.h"

class CCSSBot;

class CCSSBotSensor : public CSimplePvPSensor<CCSSBot>
{
public:
	CCSSBotSensor(CCSSBot* bot);


private:

};

#endif // !__NAVBOT_CSS_BOT_SENSOR_INTERFACE_H_
