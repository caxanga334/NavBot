#include NAVBOT_PCH_FILE
#include <algorithm>
#include <extension.h>
#include <util/entprops.h>
#include <mods/tf2/tf2lib.h>
#include <bot/basebot.h>
#include "tf2bot_weaponinfo.h"

#undef min
#undef max
#undef clamp

SourceMod::SMCResult CTF2WeaponInfoManager::ReadSMC_NewSection(const SourceMod::SMCStates* states, const char* name)
{
	if (m_parser_depth == 2)
	{
		if (strncasecmp(name, "tf_weapon_flags", 15) == 0)
		{
			m_section_tfflags = true;
			m_parser_depth++;
			return SourceMod::SMCResult::SMCResult_Continue;
		}
		else if (strncasecmp(name, "tf_data", 7) == 0)
		{
			m_section_tfdata = true;
			m_parser_depth++;
			return SourceMod::SMCResult::SMCResult_Continue;
		}
	}

	return CWeaponInfoManager::ReadSMC_NewSection(states, name);
}

SourceMod::SMCResult CTF2WeaponInfoManager::ReadSMC_KeyValue(const SourceMod::SMCStates* states, const char* key, const char* value)
{
	if (m_section_tfflags)
	{
		if (strncasecmp(key, "can_be_charged", 14) == 0)
		{
			static_cast<TF2WeaponInfo*>(m_current)->SetTF2WeaponFlag(TF2WeaponInfo::TF2_WEAPONFLAG_CHARGEMECHANIC);
		}
		else if (strncasecmp(key, "charge_is_time_based", 20) == 0)
		{
			static_cast<TF2WeaponInfo*>(m_current)->SetTF2WeaponFlag(TF2WeaponInfo::TF2_WEAPONFLAG_CHARGE_TIMEBASED);
		}
		else if (strncasecmp(key, "ubercharge_gives_crits", 22) == 0)
		{
			static_cast<TF2WeaponInfo*>(m_current)->SetTF2WeaponFlag(TF2WeaponInfo::TF2_WEAPONFLAG_UBER_IS_CRITS);
		}
		else if (strncasecmp(key, "ubercharge_gives_megaheal", 25) == 0)
		{
			static_cast<TF2WeaponInfo*>(m_current)->SetTF2WeaponFlag(TF2WeaponInfo::TF2_WEAPONFLAG_UBER_IS_MEGAHEAL);
		}
		else if (strncasecmp(key, "ubercharge_gives_dmgres", 23) == 0)
		{
			static_cast<TF2WeaponInfo*>(m_current)->SetTF2WeaponFlag(TF2WeaponInfo::TF2_WEAPONFLAG_UBER_IS_DMGRES);
		}
		else if (strncasecmp(key, "cloak_is_feign_death", 20) == 0)
		{
			static_cast<TF2WeaponInfo*>(m_current)->SetTF2WeaponFlag(TF2WeaponInfo::TF2_WEAPONFLAG_CLOAK_FEIGN_DEATH);
		}
		else
		{
			smutils->LogError(myself, "[NAVBOT TF2 WEAPON INFO] Unknown TF Weapon Flag %s at line %i col %i!", key, states->line, states->col);
			return SourceMod::SMCResult_Continue;
		}

		return SourceMod::SMCResult_Continue;
	}
	else if (m_section_tfdata)
	{
		if (strncasecmp(key, "charge_netprop_name", 19) == 0)
		{
			static_cast<TF2WeaponInfo*>(m_current)->SetChargePropertyName(value);
		}
		else if (strncasecmp(key, "charge_max", 10) == 0)
		{
			static_cast<TF2WeaponInfo*>(m_current)->SetMaxWeaponChargeAmount(atof(value));
		}
		else if (strncasecmp(key, "charge_projectile_speed_max", 27) == 0)
		{
			static_cast<TF2WeaponInfo*>(m_current)->SetMaxChargeProjectileSpeed(atof(value));
		}
		else if (strncasecmp(key, "charge_projectile_gravity_max", 29) == 0)
		{
			static_cast<TF2WeaponInfo*>(m_current)->SetMaxChargeProjectileGravity(atof(value));
		}
		else
		{
			smutils->LogError(myself, "[NAVBOT TF2 WEAPON INFO] Unknown TF Weapon Data %s - %s at line %i col %i!", key, value, states->line, states->col);
		}

		return SourceMod::SMCResult_Continue;
	}

	return CWeaponInfoManager::ReadSMC_KeyValue(states, key, value);
}

SourceMod::SMCResult CTF2WeaponInfoManager::ReadSMC_LeavingSection(const SourceMod::SMCStates* states)
{
	if (m_parser_depth == 3)
	{
		if (m_section_tfflags)
		{
			m_section_tfflags = false;
			m_parser_depth--;
			return SourceMod::SMCResult::SMCResult_Continue;
		}

		if (m_section_tfdata)
		{
			m_section_tfdata = false;
			m_parser_depth--;
			return SourceMod::SMCResult::SMCResult_Continue;
		}
	}

	return CWeaponInfoManager::ReadSMC_LeavingSection(states);
}

CTF2BotWeapon::CTF2BotWeapon(CBaseEntity* weapon) :
	CBotWeapon(weapon)
{
	if (GetTF2Info()->CanCharge())
	{
		m_charge = entprops->GetPointerToEntData<float>(weapon, Prop_Send, GetTF2Info()->GetChargePropertyName().c_str());
	}
	else
	{
		m_charge = nullptr;
	}

	if (GetTF2Info()->HasTF2WeaponFlag(TF2WeaponInfo::TF2_WEAPONFLAG_UBER_IS_CRITS))
	{
		m_ubertype = UBER_CRITS;
	}
	else if (GetTF2Info()->HasTF2WeaponFlag(TF2WeaponInfo::TF2_WEAPONFLAG_UBER_IS_MEGAHEAL))
	{
		m_ubertype = UBER_MEGAHEAL;
	}
	else if (GetTF2Info()->HasTF2WeaponFlag(TF2WeaponInfo::TF2_WEAPONFLAG_UBER_IS_DMGRES))
	{
		m_ubertype = UBER_DMGRES;
	}
	else
	{
		m_ubertype = UBER_INVUL;
	}
}

float CTF2BotWeapon::GetChargePercentage() const
{
	if (m_charge == nullptr)
	{
		return 0.0f;
	}
	else if (*m_charge < 0.01f)
	{
		return 0.0f; // not charging
	}

	auto info = GetTF2Info();

	// property holds the time the weapon began being charged (huntsman, stickybomb launcher)
	if (info->HasTF2WeaponFlag(TF2WeaponInfo::TF2_WEAPONFLAG_CHARGE_TIMEBASED))
	{
		float maxcharge = info->GetMaxWeaponChargeAmount();
		float charge = (gpGlobals->curtime - *m_charge) / info->GetMaxWeaponChargeAmount();
		charge = std::min(charge, 1.0f);
		return charge;
	}

	// property holds the charge value directly
	return *m_charge;
}

float CTF2BotWeapon::GetProjectileSpeed() const
{
	// maps the projectile speed to the charge percentage
	if (GetTF2Info()->CanCharge())
	{
		return RemapValClamped(GetChargePercentage(), 0.0f, 1.0f,
			m_info->GetAttackInfo(WeaponInfo::PRIMARY_ATTACK).GetProjectileSpeed(), GetTF2Info()->GetMaxChargeProjectileSpeed());
	}

	return m_info->GetAttackInfo(WeaponInfo::PRIMARY_ATTACK).GetProjectileSpeed();
}

float CTF2BotWeapon::GetProjectileGravity() const
{
	// maps the projectile speed to the charge percentage
	if (GetTF2Info()->CanCharge())
	{
		return RemapValClamped(GetChargePercentage(), 0.0f, 1.0f,
			m_info->GetAttackInfo(WeaponInfo::PRIMARY_ATTACK).GetGravity(), GetTF2Info()->GetMaxChargeProjectileGravity());
	}

	return m_info->GetAttackInfo(WeaponInfo::PRIMARY_ATTACK).GetGravity();
}

bool CTF2BotWeapon::IsDeployedOrScoped(const CBaseBot* owner) const
{
	CBaseEntity* me = owner->GetEntity();
	return tf2lib::IsPlayerInCondition(me, TeamFortress2::TFCond::TFCond_Slowed);
}
