#ifndef TF2BOT_WEAPON_INFO_H_
#define TF2BOT_WEAPON_INFO_H_

#include <cinttypes>
#include <string>
#include <bot/interfaces/weaponinfo.h>
#include <bot/interfaces/weapon.h>

class TF2WeaponInfo : public WeaponInfo
{
public:
	TF2WeaponInfo()
	{
		m_tfweaponflags = static_cast<std::int32_t>(TF2_WEAPONFLAG_NONE);
		m_tfmaxcharge = 0.0f;
		m_tfprojspeedatmaxcharge = 0.0f;
		m_tfprojgravatmaxcharge = 0.0f;
	}

	enum TF2WeaponInfoFlags : std::int32_t
	{
		TF2_WEAPONFLAG_NONE = 0,
		TF2_WEAPONFLAG_CHARGEMECHANIC = (1 << 0), // weapon can be charged by holding the attack button (huntsman, stickybomb launcher)
		TF2_WEAPONFLAG_CHARGE_TIMEBASED = (1 << 1), // the net prop gives us the time the weapon started being charged
		TF2_WEAPONFLAG_UBER_IS_CRITS = (1 << 2),
		TF2_WEAPONFLAG_UBER_IS_MEGAHEAL = (1 << 3),
		TF2_WEAPONFLAG_UBER_IS_DMGRES = (1 << 4),
		TF2_WEAPONFLAG_CLOAK_FEIGN_DEATH = (1 << 5),
	};

	void SetTF2WeaponFlag(TF2WeaponInfoFlags flags)
	{
		m_tfweaponflags |= flags;
	}

	void ClearTF2WeaponFlag(TF2WeaponInfoFlags flags)
	{
		m_tfweaponflags &= ~flags;
	}

	bool HasTF2WeaponFlag(TF2WeaponInfoFlags flags) const
	{
		return (m_tfweaponflags & flags) != 0 ? true : false;
	}

	bool CanCharge() const { return HasTF2WeaponFlag(TF2_WEAPONFLAG_CHARGEMECHANIC); }

	void SetChargePropertyName(const char* name) { m_chargenetpropname.assign(name); }
	const std::string& GetChargePropertyName() const { return m_chargenetpropname; }

	void SetMaxWeaponChargeAmount(float maxc) { m_tfmaxcharge = maxc; }
	const float GetMaxWeaponChargeAmount() const { return m_tfmaxcharge; }

	void SetMaxChargeProjectileSpeed(float speed) { m_tfprojspeedatmaxcharge = speed; }
	const float GetMaxChargeProjectileSpeed() const { return m_tfprojspeedatmaxcharge; }

	void SetMaxChargeProjectileGravity(float grav) { m_tfprojgravatmaxcharge = grav; }
	const float GetMaxChargeProjectileGravity() const { return m_tfprojgravatmaxcharge; }

private:
	std::int32_t m_tfweaponflags;
	float m_tfmaxcharge;
	float m_tfprojspeedatmaxcharge;
	float m_tfprojgravatmaxcharge;
	std::string m_chargenetpropname;
};

class CTF2WeaponInfoManager : public CWeaponInfoManager
{
public:
	CTF2WeaponInfoManager()
	{
		m_section_tfflags = false;
		m_section_tfdata = false;
	}

protected:
	WeaponInfo* CreateWeaponInfo() const override { return  static_cast<WeaponInfo*>(new TF2WeaponInfo); }

	SourceMod::SMCResult ReadSMC_NewSection(const SourceMod::SMCStates* states, const char* name) override;
	SourceMod::SMCResult ReadSMC_KeyValue(const SourceMod::SMCStates* states, const char* key, const char* value) override;
	SourceMod::SMCResult ReadSMC_LeavingSection(const SourceMod::SMCStates* states) override;

private:
	bool m_section_tfflags;
	bool m_section_tfdata;
};

class CTF2BotWeapon : public CBotWeapon
{
public:
	CTF2BotWeapon(CBaseEntity* weapon);

	enum UberType : std::int32_t
	{
		UBER_INVUL = 0, // Stock medigun
		UBER_CRITS, // Kritzkrieg
		UBER_MEGAHEAL, // Quick-fix
		UBER_DMGRES, // Vaccinator
	};

	float GetChargePercentage() const;
	// takes into account the weapon charge mechanic (IE: huntsman)
	float GetProjectileSpeed(WeaponInfo::AttackFunctionType type) const;
	// takes into account the weapon charge mechanic (IE: huntsman)
	float GetProjectileGravity(WeaponInfo::AttackFunctionType type) const;
	const TF2WeaponInfo* GetTF2Info() const { return static_cast<const TF2WeaponInfo*>(GetWeaponInfo()); }

	UberType GetMedigunUberType() const { return m_ubertype; }

	bool IsDeployedOrScoped(const CBaseBot* owner) const override;

	bool IsTheRescueRanger() const { return IsWeapon("tf_weapon_shotgun_building_rescue"); }

private:
	float* m_charge;
	UberType m_ubertype;
};

#endif

