#ifndef __NAVBOT_CSS_BOT_WEAPON_H_
#define __NAVBOT_CSS_BOT_WEAPON_H_

#include "../interfaces/weapon.h"

class CCSSBotWeapon : public CBotWeapon
{
public:
	CCSSBotWeapon(CBaseEntity* entity);

	bool IsDeployedOrScoped(const CBaseBot* owner) const override;
};

#endif // !__NAVBOT_CSS_BOT_WEAPON_H_
