#include NAVBOT_PCH_FILE
#include <sdkports/sdk_entityoutput.h>
#include "gamedata_const.h"
#include <in_buttons.h>

#define INIT_BUTTON(VAR, VALUE)		\
	if (this->VAR < 0)				\
	{								\
		this->VAR = VALUE;			\
	}								\
									\

#define GET_BUTTON_FROM_GAMEDATA(VAR)							\
	value = gamedata->GetKeyValue("UserCmdButton_"#VAR);		\
																\
	if (value)													\
	{															\
		this->VAR = atoi(value);								\
	}															\


void GamedataConstants::UserButtons::InitFromSDK()
{
	/*
	* All buttons are initialized with -1.
	* This function is called after we initialized the buttons with values from the gamedata files.
	* Any button that still has a value of -1 should either be set to a value from the SDK or set to 0.
	*/

	/*
	* 	int forward;
		int backward;
		int moveleft;
		int moveright;
		int crouch;
		int jump;
		int speed;
		int walk;
		int use;
		int reload;
		int zoom;
	*/

	INIT_BUTTON(attack1, IN_ATTACK);
	INIT_BUTTON(attack2, IN_ATTACK2);

#if defined(IN_ATTACK3)
	INIT_BUTTON(attack3, IN_ATTACK3);
#else
	INIT_BUTTON(attack3, 0);
#endif

	INIT_BUTTON(forward, IN_FORWARD);
	INIT_BUTTON(backward, IN_BACK);
	INIT_BUTTON(moveleft, IN_MOVELEFT);
	INIT_BUTTON(moveright, IN_MOVERIGHT);
	INIT_BUTTON(crouch, IN_DUCK);
	INIT_BUTTON(jump, IN_JUMP);
	INIT_BUTTON(speed, IN_SPEED);
	INIT_BUTTON(run, IN_RUN);
	INIT_BUTTON(walk, IN_WALK);
	INIT_BUTTON(use, IN_USE);
	INIT_BUTTON(reload, IN_RELOAD);
	INIT_BUTTON(zoom, IN_ZOOM);
	INIT_BUTTON(alt1, IN_ALT1);
	INIT_BUTTON(modcustom1, 0);
	INIT_BUTTON(modcustom2, 0);
	INIT_BUTTON(modcustom3, 0);
	INIT_BUTTON(modcustom4, 0);
}

void GamedataConstants::UserButtons::InitFromGamedata(SourceMod::IGameConfig* gamedata)
{
	std::memset(this, -1, sizeof(GamedataConstants::UserButtons));
	const char* value = nullptr;

	// the key name is UserCmdButton_VAR
	// Example: UserCmdButton_forward

	GET_BUTTON_FROM_GAMEDATA(attack1);
	GET_BUTTON_FROM_GAMEDATA(attack2);
	GET_BUTTON_FROM_GAMEDATA(attack3);
	GET_BUTTON_FROM_GAMEDATA(forward);
	GET_BUTTON_FROM_GAMEDATA(backward);
	GET_BUTTON_FROM_GAMEDATA(moveleft);
	GET_BUTTON_FROM_GAMEDATA(moveright);
	GET_BUTTON_FROM_GAMEDATA(crouch);
	GET_BUTTON_FROM_GAMEDATA(jump);
	GET_BUTTON_FROM_GAMEDATA(speed);
	GET_BUTTON_FROM_GAMEDATA(run);
	GET_BUTTON_FROM_GAMEDATA(walk);
	GET_BUTTON_FROM_GAMEDATA(use);
	GET_BUTTON_FROM_GAMEDATA(reload);
	GET_BUTTON_FROM_GAMEDATA(zoom);
	GET_BUTTON_FROM_GAMEDATA(alt1);
	GET_BUTTON_FROM_GAMEDATA(modcustom1);
	GET_BUTTON_FROM_GAMEDATA(modcustom2);
	GET_BUTTON_FROM_GAMEDATA(modcustom3);
	GET_BUTTON_FROM_GAMEDATA(modcustom4);
}

bool GamedataConstants::Initialize(SourceMod::IGameConfig* gamedata)
{
	const char* buffer = gamedata->GetKeyValue("BotAim_HeadBoneName");

	if (buffer)
	{
		s_head_aim_bone.assign(buffer);
	}

	s_user_buttons.InitFromGamedata(gamedata);
	s_user_buttons.InitFromSDK();

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
				smutils->LogError(myself, "GamedataConstants::OnMapStart got NULL for worldspawn entity!");
			}
		}

		init = true;
	}
}
