#ifndef NAVBOT_BLACK_MESA_BOT_INVENTORY_INTERFACE_H_
#define NAVBOT_BLACK_MESA_BOT_INVENTORY_INTERFACE_H_

#include <string>
#include <bot/interfaces/inventory.h>
#include <mods/blackmesa/blackmesa_shareddefs.h>

class CBlackMesaBot;

class CBlackMesaBotInventory : public IInventory
{
public:
	CBlackMesaBotInventory(CBlackMesaBot* bot);
	~CBlackMesaBotInventory() override;

	/**
	 * @brief Checks if the bot owns this item weapon (item_weapon_*)
	 * @param itemname Item weapon classname.
	 * @return true if the bot owns the weapon. false otherwise.
	 */
	bool OwnsThisWeapon(const std::string& itemname);
};

#endif // !NAVBOT_BLACK_MESA_BOT_INVENTORY_INTERFACE_H_
