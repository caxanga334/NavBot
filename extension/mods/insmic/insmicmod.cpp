#include NAVBOT_PCH_FILE
#include <tier1/KeyValues.h>
#include <bot/insmic/insmicbot.h>
#include "insmic_modhelpers.h"
#include "insmicmod.h"

CInsMICMod* CInsMICMod::GetInsurgencyMod()
{
	return static_cast<CInsMICMod*>(extmanager->GetMod());
}

CInsMICMod::CInsMICMod()
{
	m_gametype = insmic::InsMICGameTypes_t::GAMETYPE_INVALID;
}

CBaseBot* CInsMICMod::AllocateBot(edict_t* edict)
{
	return new CInsMICBot(edict);
}

IModHelpers* CInsMICMod::AllocModHelpers() const
{
	return new CInsMICModHelpers;
}

void CInsMICMod::SelectTeamAndSquadForBot(int& teamid, int& encodedSquadData) const
{
	int t1c, t2c;
	insmiclib::GetTeamClientCount(t1c, t2c);

	if (t1c > t2c)
	{
		teamid = static_cast<int>(insmic::InsMICTeam::INSMICTEAM_INSURGENTS);
	}
	else
	{
		teamid = static_cast<int>(insmic::InsMICTeam::INSMICTEAM_USMC);
	}

	int slot = insmic::INVALID_SLOT;
	int squadid = static_cast<int>(insmic::InsMICSquads_t::INSMIC_INVALID_SQUAD);

	auto func = [&teamid, &slot, &squadid](int index, edict_t* edict, CBaseEntity* entity) {
		if (entity)
		{
			bool enabled = false;
			entprops->GetEntPropBool(entity, Prop_Send, "m_bEnabled", enabled);
			int teamnum = static_cast<int>(insmic::InsMICTeam::INSMICTEAM_UNASSINGED);
			entprops->GetEntProp(entity, Prop_Send, "m_iParentTeam", teamnum);

			if (enabled && teamnum == teamid)
			{
				if (insmiclib::GetRandomFreeSquadSlot(entity, slot))
				{
					squadid = insmiclib::GetSquadID(entity);
					return false; // stop the loop
				}
			}
		}

		return true;
	};

	UtilHelpers::ForEachEntityOfClassname("ins_squad", func);

	if (slot == insmic::INVALID_SLOT)
	{
		encodedSquadData = insmic::INVALID_SLOT;
		return;
	}

	teamid--;
	encodedSquadData = insmiclib::EncodeSquadData(squadid, slot);
}

void CInsMICMod::OnMapStart()
{
	CBaseMod::OnMapStart();

	DetectGameType();
}

static void InsMICCommand_DebugSquadIDs(const CConCommandArgs& args)
{
	auto func = [](int index, edict_t* edict, CBaseEntity* entity) {
		if (entity)
		{
			bool enabled = false;
			entprops->GetEntPropBool(entity, Prop_Send, "m_bEnabled", enabled);
			int teamid = static_cast<int>(insmic::InsMICTeam::INSMICTEAM_UNASSINGED);
			entprops->GetEntProp(entity, Prop_Send, "m_iParentTeam", teamid);
			char name[256];
			size_t len = 0;
			std::memset(name, 0, sizeof(name));
			entprops->GetEntPropString(entity, Prop_Send, "m_szName", name, sizeof(name), len);
			int squadid = insmiclib::GetSquadID(entity);

			META_CONPRINTF("Squad %s\n  Enabled: %s\n  ID: %i\n  Team: %i\n  Name: %s\n", 
				UtilHelpers::textformat::FormatEntity(entity), UtilHelpers::textformat::FormatBool(enabled), squadid, teamid, name);
		}

		return true;
	};

	UtilHelpers::ForEachEntityOfClassname("ins_squad", func);
}

void CInsMICMod::RegisterModCommands()
{
	CServerCommandManager& manager = extmanager->GetServerCommandManager();

	manager.RegisterConCommand("sm_navbot_insmic_debug_squad_ids",
		"Prints the squad manager entity's Squad IDs to the console.", FCVAR_GAMEDLL | FCVAR_CHEAT, InsMICCommand_DebugSquadIDs);
}

void CInsMICMod::DetectGameType()
{
	using namespace std::literals::string_view_literals;
	constexpr auto CONFIG_FILE_EXT = "imc2"sv;
	char path[256];

	ke::SafeSprintf(path, sizeof(path), "maps/%s.%s", STRING(gpGlobals->mapname), CONFIG_FILE_EXT.data());
	m_gametype = insmic::InsMICGameTypes_t::GAMETYPE_TDM; // default to TDM

	if (filesystem->FileExists(path, "MOD"))
	{
		KeyValues* root = new KeyValues("IMCData");
		
		if (!root->LoadFromFile(filesystem, path, "MOD"))
		{
			root->deleteThis();
			smutils->LogError(myself, "Failed to load IMCConfig file \"%s\"!", path);
			return;
		}

		KeyValues* profiledata = root->FindKey("ProfileData");

		if (profiledata)
		{
			for (KeyValues* sub = profiledata->GetFirstSubKey(); sub != nullptr; sub = sub->GetNextKey())
			{
				const char* gamemode = sub->GetString("gametype", nullptr);

				if (gamemode)
				{
					auto gametype = insmiclib::StringToGameType(gamemode);
					m_gametype = gametype != insmic::InsMICGameTypes_t::GAMETYPE_INVALID ? gametype : insmic::InsMICGameTypes_t::GAMETYPE_TDM;
					root->deleteThis();
					return;
				}
			}
		}

		root->deleteThis();
	}
}
