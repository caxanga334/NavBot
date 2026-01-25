#include NAVBOT_PCH_FILE
#include "insmicbot.h"
#include "insmicbot_sensor.h"

CInsMICBotSensor::CInsMICBotSensor(CInsMICBot* bot) :
	CSimplePvPSensor<CInsMICBot>(bot)
{
}
