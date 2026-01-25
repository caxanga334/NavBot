#include NAVBOT_PCH_FILE
#include "insmicmod.h"

int insmiclib::GetSquadID(CBaseEntity* pEntity)
{
	static int offset = 0;

	EXT_ASSERT(entprops->HasEntProp(pEntity, Prop_Send, "m_iParentTeam"), "insmiclib::GetSquadID given entity is not an instance of CINSSquad!");

	if (offset == 0)
	{
		SourceMod::IGameConfig* gamedata = extension->GetExtensionGameData();
		
		int offs = 0;
		
		if (!gamedata->GetOffset("CINSSquad::m_iID", &offs))
		{
			smutils->LogError(myself, "Offset \"CINSSquad::m_iID\" not found on NavBot's gamedata!");
			return 0;
		}

		SourceMod::sm_sendprop_info_t info;

		if (!gamehelpers->FindSendPropInfo("CINSSquad", "m_iParentTeam", &info))
		{
			smutils->LogError(myself, "Networked property offset \"CINSSquad::m_iParentTeam\" not found!");
			return 0;
		}

		offset = static_cast<int>(info.actual_offset) + offs;
	}

	int* id = entprops->GetPointerToEntData<int>(pEntity, static_cast<unsigned int>(offset));
	return *id;
}

void insmiclib::GetTeamClientCount(int& teamone, int& teamtwo)
{
	teamone = 0;
	teamtwo = 0;

	auto func = [&teamone, &teamtwo](int index, edict_t* edict, CBaseEntity* entity) {
		if (entity)
		{
			bool enabled = false;
			entprops->GetEntPropBool(entity, Prop_Send, "m_bEnabled", enabled);

			if (!enabled)
			{
				return true;
			}

			int teamid = static_cast<int>(insmic::InsMICTeam::INSMICTEAM_UNASSINGED);
			entprops->GetEntProp(entity, Prop_Send, "m_iParentTeam", teamid);

			if (teamid == static_cast<int>(insmic::InsMICTeam::INSMICTEAM_UNASSINGED) || teamid == static_cast<int>(insmic::InsMICTeam::INSMICTEAM_SPECTATOR))
			{
				return true;
			}

			int slot = insmic::INVALID_SLOT;

			for (int i = 0; i < insmic::MAX_SQUAD_SLOTS; i++)
			{
				entprops->GetEntProp(entity, Prop_Send, "m_iPlayerSlots", slot, 4, i);

				if (slot != insmic::INVALID_SLOT)
				{
					if (teamid == static_cast<int>(insmic::InsMICTeam::INSMICTEAM_USMC))
					{
						teamone++;
					}
					else
					{
						teamtwo++;
					}
				}
			}
		}

		return true;
	};

	UtilHelpers::ForEachEntityOfClassname("ins_squad", func);
}

bool insmiclib::GetRandomFreeSquadSlot(CBaseEntity* pEntity, int& slot)
{
	slot = insmic::INVALID_SLOT;
	bool enabled = false;
	entprops->GetEntPropBool(pEntity, Prop_Send, "m_bEnabled", enabled);

	if (!enabled)
	{
		return false;
	}

	int teamid = static_cast<int>(insmic::InsMICTeam::INSMICTEAM_UNASSINGED);
	entprops->GetEntProp(pEntity, Prop_Send, "m_iParentTeam", teamid);

	if (teamid == static_cast<int>(insmic::InsMICTeam::INSMICTEAM_UNASSINGED) || teamid == static_cast<int>(insmic::InsMICTeam::INSMICTEAM_SPECTATOR))
	{
		return false;
	}

	int islot = insmic::INVALID_SLOT;
	std::array<int, insmic::MAX_SQUAD_SLOTS> freeslots;
	std::fill(freeslots.begin(), freeslots.end(), insmic::INVALID_SLOT);
	int count = 0;

	for (int i = 0; i < insmic::MAX_SQUAD_SLOTS; i++)
	{
		entprops->GetEntProp(pEntity, Prop_Send, "m_iPlayerSlots", islot, 4, i);

		if (islot == insmic::INVALID_SLOT)
		{
			freeslots[count] = i;
			count++;
		}
	}

	if (count == 0)
	{
		return false;
	}

	if (count == 1)
	{
		slot = freeslots[0];
		return true;
	}

	slot = freeslots[randomgen->GetRandomInt<int>(0, count - 1)];
	return true;
}

insmic::InsMICGameTypes_t insmiclib::StringToGameType(const char* str)
{
	using namespace std::literals::string_view_literals;

	constexpr std::array lookuparray = {
		"GAMETYPE_BATTLE"sv,
		"GAMETYPE_FIREFIGHT"sv,
		"GAMETYPE_PUSH"sv,
		"GAMETYPE_STRIKE"sv,
		"GAMETYPE_TDM"sv,
		"GAMETYPE_POWERBALL"sv,
	};

	static_assert(lookuparray.size() == static_cast<size_t>(insmic::InsMICGameTypes_t::MAX_GAMETYPES), "Insurgency game type and look up array size mismatch!");

	for (int i = 0; i < static_cast<int>(lookuparray.size()); i++)
	{
		if (ke::StrCaseCmp(str, lookuparray[i].data()) == 0)
		{
			return static_cast<insmic::InsMICGameTypes_t>(i);
		}
	}

	return insmic::InsMICGameTypes_t::GAMETYPE_INVALID;
}
