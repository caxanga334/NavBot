#include NAVBOT_PCH_FILE
#include <bot/basebot.h>
#include "misc.h"

class NavBotTargetProcessor : public SourceMod::ICommandTargetProcessor
{
public:
	// Inherited via ICommandTargetProcessor
	bool ProcessCommandTarget(SourceMod::cmd_target_info_t* info) override
	{
		if (std::strcmp(info->pattern, "@navbot") == 0 || std::strcmp(info->pattern, "@navbots") == 0)
		{
			ke::SafeStrcpy(info->target_name, info->target_name_maxlength, "NavBots");
			info->target_name_style = COMMAND_TARGETNAME_RAW;
			info->num_targets = 0;

			// This is a multitarget filter
			if ((info->flags & COMMAND_FILTER_NO_BOTS) != 0 || (info->flags & COMMAND_FILTER_NO_MULTI) != 0)
			{
				
				info->reason = COMMAND_TARGET_EMPTY_FILTER;
				return true;
			}

			info->reason = COMMAND_TARGET_VALID;

			auto& thebots = extmanager->GetAllBots();

			if (thebots.empty())
			{
				info->reason = COMMAND_TARGET_EMPTY_FILTER;
				return true;
			}

			for (auto& ptr : thebots)
			{
				const CBaseBot* bot = ptr.get();

				if ((info->flags & COMMAND_FILTER_ALIVE) != 0)
				{
					if (!bot->IsAlive())
					{
						continue;
					}
				}

				if ((info->flags & COMMAND_FILTER_DEAD) != 0)
				{
					if (bot->IsAlive())
					{
						continue;
					}
				}

				info->targets[info->num_targets] = bot->GetIndex();
				info->num_targets++;

				if (info->num_targets >= static_cast<unsigned int>(info->max_targets))
				{
					break;
				}
			}

			// no target found and the bot vector is not empty
			if (info->num_targets == 0)
			{
				if ((info->flags & COMMAND_FILTER_ALIVE) != 0)
				{
					info->reason = COMMAND_TARGET_NOT_ALIVE;
				}
				else if ((info->flags & COMMAND_FILTER_DEAD) != 0)
				{
					info->reason = COMMAND_TARGET_NOT_DEAD;
				}
				else
				{
					info->reason = COMMAND_TARGET_EMPTY_FILTER;
				}

				return true;
			}

			info->reason = COMMAND_TARGET_VALID;

			return true;
		}

		return false;
	}
};

static NavBotTargetProcessor s_navbottargetprocessor;

void spmisc::RegisterNavBotMultiTargetFilter()
{
	playerhelpers->RegisterCommandTargetProcessor(&s_navbottargetprocessor);
}

void spmisc::UnregisterNavBotMultiTargetFilter()
{
	playerhelpers->UnregisterCommandTargetProcessor(&s_navbottargetprocessor);
}
