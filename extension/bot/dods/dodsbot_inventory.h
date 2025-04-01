#ifndef __NAVBOT_DODSBOT_INVENTORY_INTERFACE_H_
#define __NAVBOT_DODSBOT_INVENTORY_INTERFACE_H_

#include <bot/interfaces/inventory.h>

class CDoDSBotInventory : public IInventory
{
public:
	CDoDSBotInventory(CBaseBot* bot);
	~CDoDSBotInventory() override;

	bool HasBomb() const;

private:

};


#endif // !__NAVBOT_DODSBOT_INVENTORY_INTERFACE_H_
