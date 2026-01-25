#ifndef __NAVBOT_INSMICBOT_SENSOR_INTERFACE_H_
#define __NAVBOT_INSMICBOT_SENSOR_INTERFACE_H_

#include <bot/interfaces/sensor_templates.h>

class CInsMICBot;

class CInsMICBotSensor : public CSimplePvPSensor<CInsMICBot>
{
public:
	CInsMICBotSensor(CInsMICBot* bot);


private:

};

#endif // !__NAVBOT_INSMICBOT_SENSOR_INTERFACE_H_
