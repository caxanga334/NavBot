#include <extension.h>
#include "bmbot.h"
#include "bmbot_inventory.h"

CBlackMesaBotInventory::CBlackMesaBotInventory(CBlackMesaBot* bot) :
	IInventory(bot)
{
}

CBlackMesaBotInventory::~CBlackMesaBotInventory()
{
}

bool CBlackMesaBotInventory::OwnsThisWeapon(const std::string& itemname)
{
	std::size_t it = itemname.find("item_");

	if (it == std::string::npos)
	{
		return false;
	}

	constexpr std::size_t item_length = 5U; // length of item_
	std::string weaponname = itemname.substr(it + 5U);
	return HasWeapon(weaponname.c_str());
}
