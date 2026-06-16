#include NAVBOT_PCH_FILE
#include <mods/insmic/insmicmod.h>
#include "insmicbot.h"
#include "insmicbot_sensor.h"

CInsMICBotSensor::CInsMICBotSensor(CInsMICBot* bot) :
	CSimplePvPSensor<CInsMICBot>(bot)
{
}
