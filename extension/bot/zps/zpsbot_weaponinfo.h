#ifndef __NAVBOT_ZPSBOT_WEAPON_INFO_H_
#define __NAVBOT_ZPSBOT_WEAPON_INFO_H_

#include <bot/interfaces/weaponinfo.h>
#include "zpsbot_weapon.h"

class ZPSWeaponInfo : public WeaponInfo
{
public:
	ZPSWeaponInfo() :
		WeaponInfo()
	{
		m_size = 1;
	}

	void VariantOf(const WeaponInfo* other) override
	{
		WeaponInfo::VariantOf(other);
		this->m_size = static_cast<const ZPSWeaponInfo*>(other)->m_size;
	}

	void SetInventorySize(int size) { m_size = size; }
	int GetInventorySize() const { return m_size; }

private:
	int m_size;
};

class CZPSWeaponInfoManager : public CWeaponInfoManager
{
public:
	CZPSWeaponInfoManager() :
		CWeaponInfoManager()
	{
		m_inzpsdatasection = false;
	}

protected:
	void ReadSMC_ParseStart() override
	{
		CWeaponInfoManager::ReadSMC_ParseStart();
		m_inzpsdatasection = false;
	}
	SourceMod::SMCResult ReadSMC_NewSection(const SourceMod::SMCStates* states, const char* name) override;
	SourceMod::SMCResult ReadSMC_KeyValue(const SourceMod::SMCStates* states, const char* key, const char* value) override;
	SourceMod::SMCResult ReadSMC_LeavingSection(const SourceMod::SMCStates* states) override;
	WeaponInfo* CreateWeaponInfo() const override { return new ZPSWeaponInfo; }
	ZPSWeaponInfo* GetCurrentZPSInfo() const { return static_cast<ZPSWeaponInfo*>(m_current); }

private:
	bool m_inzpsdatasection;
};

#endif // !__NAVBOT_ZPSBOT_WEAPON_INFO_H_
