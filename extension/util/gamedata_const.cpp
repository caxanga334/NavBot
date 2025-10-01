#include NAVBOT_PCH_FILE
#include <sdkports/sdk_entityoutput.h>
#include "gamedata_const.h"

bool GamedataConstants::Initialize(SourceMod::IGameConfig* gamedata)
{
	const char* buffer = gamedata->GetKeyValue("BotAim_HeadBoneName");

	if (buffer)
	{
		s_head_aim_bone.assign(buffer);
	}


	return true;
}

void GamedataConstants::OnMapStart(SourceMod::IGameConfig* gamedata, const bool force)
{
	static bool init = false;

	if (!init || force)
	{
		int size = 0;

		if (gamedata->GetOffset("SizeOf_COutputEvent", &size))
		{
			// use value from gamedata
			s_sizeof_COutputEvent = static_cast<std::size_t>(size);
		}
		else
		{
			// calculate the value from the worldspawn entity
			CBaseEntity* worldspawn = gamehelpers->ReferenceToEntity(0);

			if (worldspawn)
			{
				/*
					int g_m_pStudioHdr = 0;

					public void OnMapStart() {
						if (g_sizeof_m_pStudioHdr == 0) {
							int prop = CreateEntityByName("prop_dynamic");
							int sizeof_COutputEvent = FindDataMapInfo(prop, "m_OnUser2") - FindDataMapInfo(prop, "m_OnUser1");
							g_m_pStudioHdr = FindDataMapInfo(prop, "m_OnIgnite") + sizeof_COutputEvent;
						}
					}
				*/

				datamap_t* dmap = gamehelpers->GetDataMap(worldspawn);

				SourceMod::sm_datatable_info_t onuser1;
				SourceMod::sm_datatable_info_t onuser2;

				if (gamehelpers->FindDataMapInfo(dmap, "m_OnUser1", &onuser1) && gamehelpers->FindDataMapInfo(dmap, "m_OnUser2", &onuser2))
				{
					s_sizeof_COutputEvent = static_cast<std::size_t>(onuser2.actual_offset - onuser1.actual_offset);

#ifdef EXT_DEBUG
					smutils->LogMessage(myself, "sizeof(COutputEvent) is %zu", s_sizeof_COutputEvent);

					// On public SDKs, the size of COutputEvent should match the size of CBaseEntityOutput.
					// This will warn us if members were added or removed
					if (sizeof(CBaseEntityOutput) != s_sizeof_COutputEvent)
					{
						smutils->LogError(myself, "COutputEvent size doesn't match the size of CBaseEntityOutput! %zu != %u", s_sizeof_COutputEvent, sizeof(CBaseEntityOutput));
					}
#endif // EXT_DEBUG
				}
				else
				{
					smutils->LogError(myself, "Failed to get datamap info for m_OnUser1 and m_OnUser2 of worldspawn entity!");
				}
			}
			else
			{
				smutils->LogError(myself, "gdc::OnMapStart got NULL for worldspawn entity!");
			}
		}
	}
}
