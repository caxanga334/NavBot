#include NAVBOT_PCH_FILE
#include <mods/tf2/teamfortress2mod.h>
#include <mods/tf2/tf2lib.h>
#include "tf2bot.h"
#include "tf2bot_combat.h"

CTF2BotCombat::CTF2BotCombat(CBaseBot* bot) :
	ICombat(bot)
{

}

bool CTF2BotCombat::IsAbleToDodgeEnemies(const CKnownEntity* threat, const CBotWeapon* activeWeapon)
{
	CTF2Bot* bot = GetBot<CTF2Bot>();
	CBaseEntity* pEntity = bot->GetEntity();

	if (bot->GetMyClassType() == TeamFortress2::TFClassType::TFClass_Engineer)
	{
		return false;
	}

	// Don't dodge while invisible or invulnerable
	if (tf2lib::IsPlayerInvisible(pEntity) || tf2lib::IsPlayerInvulnerable(bot->GetIndex()))
	{
		return false;
	}

	// Don't dodge while disguised
	if (tf2lib::IsPlayerInCondition(pEntity, TeamFortress2::TFCond::TFCond_Disguised))
	{
		return false;
	}

	return ICombat::IsAbleToDodgeEnemies(threat, activeWeapon);
}

bool CTF2BotCombat::CanFireWeaponAtEnemy(const CBaseBot* bot, const CKnownEntity* threat, const CBotWeapon* activeWeapon) const
{
	if (tf2lib::IsPlayerInCondition(bot->GetEntity(), TeamFortress2::TFCond::TFCond_Taunting))
		return false;

	return ICombat::CanFireWeaponAtEnemy(bot, threat, activeWeapon);
}

bool CTF2BotCombat::HandleWeapon(const CBaseBot* bot, const CBotWeapon* activeWeapon)
{
	const CTF2BotWeapon* tfweapon = static_cast<const CTF2BotWeapon*>(activeWeapon);

	if (tfweapon->GetTF2Info()->CanCharge())
	{
		if (tfweapon->GetChargePercentage() >= 0.99f)
		{
			bot->GetControlInterface()->ReleaseAllAttackButtons();
			return false; // block attack
		}
	}

	return ICombat::HandleWeapon(bot, activeWeapon);
}

void CTF2BotCombat::UseSecondaryAbilities(const CKnownEntity* threat, const CBotWeapon* activeWeapon)
{
	if (!CanUseSecondaryAbilitities()) { return; }

	CTF2Bot* bot = GetBot<CTF2Bot>();

	switch (bot->GetMyClassType())
	{
	case TeamFortress2::TFClassType::TFClass_Pyro:
	{
		PyroFireCompressionBlast(threat, activeWeapon);
		break;
	}
	case TeamFortress2::TFClassType::TFClass_DemoMan:
	{
		DemomanUseShieldCharge(threat);
		break;
	}
	default:
		break;
	}

	GetSecondaryAbilitiesTimer().Start(bot->GetDifficultyProfile()->GetAbilityUsageInterval());
}

void CTF2BotCombat::PyroFireCompressionBlast(const CKnownEntity* threat, const CBotWeapon* activeWeapon)
{
	CTF2Bot* bot = GetBot<CTF2Bot>();

	if (bot->GetDifficultyProfile()->GetWeaponSkill() < 25) { return; }

	if (!activeWeapon->GetWeaponInfo()->HasTag("can_airblast"))
	{
		return; // can't airblast with this weapon
	}

	// scan for entities in front of the pyro

	Vector forward;
	bot->EyeVectors(&forward);
	Vector center = bot->GetEyeOrigin() + (forward * 64.0f);
	Vector size = { 96.0f, 96.0f, 64.0f };
	UtilHelpers::CEntityEnumerator enumerator;

	UtilHelpers::EntitiesInBox(center - size, center + size, enumerator);

	bool airblast = false;
	auto myteam = bot->GetMyTFTeam();
	auto functor = [&myteam, &airblast](CBaseEntity* entity) -> bool {

		if (airblast)
		{
			return false;
		}

		if (entity)
		{
			const char* classname = gamehelpers->GetEntityClassname(entity);
			auto team = tf2lib::GetEntityTFTeam(entity);

			if (team == myteam)
			{
				return true;
			}

			if (std::strcmp(classname, "player") == 0 && CBaseBot::s_botrng.GetRandomChance(15))
			{
				if (static_cast<int>(team) <= TEAM_SPECTATOR || !UtilHelpers::IsPlayerAlive(UtilHelpers::IndexOfEntity(entity)))
				{
					return true;
				}

				airblast = true;
			}
			else if (std::strncmp(classname, "tf_projectile", 13) == 0)
			{
				airblast = true;
			}
		}

		return true;
	};

	enumerator.ForEach(functor);

	if (bot->IsDebugging(BOTDEBUG_COMBAT))
	{
		NDebugOverlay::SweptBox(center, center, -size, size, bot->GetEyeAngles(), 255, 255, 0, 200, 0.5f);
	}

	if (airblast)
	{
		bot->GetControlInterface()->ReleaseAllAttackButtons();
		bot->GetControlInterface()->PressSecondaryAttackButton();
	}
}

void CTF2BotCombat::DemomanUseShieldCharge(const CKnownEntity* threat)
{
	CTF2Bot* bot = GetBot<CTF2Bot>();
	CTF2BotPlayerController* control = bot->GetControlInterface();

	// low aggressiveness bots only charge at full health
	if (bot->GetDifficultyProfile()->GetAggressiveness() < 50 && bot->GetHealthPercentage() < 0.7f)
	{
		return;
	}

	bool hasShield = false;
	entprops->GetEntPropBool(bot->GetEntity(), Prop_Send, "m_bShieldEquipped", hasShield);
	float chargeMeter = 0.0f;
	entprops->GetEntPropFloat(bot->GetEntity(), Prop_Send, "m_flChargeMeter", chargeMeter);

	// has visible enemy, can charge and is currently aiming on the enemy
	if (hasShield && threat->IsVisibleNow() && chargeMeter >= 99.8f && control->IsAimOnTarget())
	{
		// charge
		control->PressSecondaryAttackButton();
	}
}

