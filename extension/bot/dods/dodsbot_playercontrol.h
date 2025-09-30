#ifndef __NAVBOT_DODSBOT_PLAYER_CONTROLLER_H_
#define __NAVBOT_DODSBOT_PLAYER_CONTROLLER_H_

#include <bot/interfaces/playercontrol.h>

class CDoDSBotPlayerController : public IPlayerController
{
public:
	CDoDSBotPlayerController(CBaseBot* bot);

	inline void PressProneButton(const float duration = -1.0f) { this->PressAlt1Button(duration); }
	inline void ReleaseProneButton() { this->ReleaseAlt1Button(); }

private:

};

#endif // !__NAVBOT_DODSBOT_PLAYER_CONTROLLER_H_
