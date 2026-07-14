#include NAVBOT_PCH_FILE
#include <util/pawnutils.h>

namespace natives::utils
{
	static cell_t GetEntityWorldSpaceCenter(IPluginContext* context, const cell_t* params)
	{
		CBaseEntity* entity = pawnutils::ReadEntity(context, params, 1);

		if (!entity)
		{
			return 0;
		}

		pawnutils::WriteVector(context, params, 2, UtilHelpers::getWorldSpaceCenter(entity));
		return 0;
	}

	void setup(std::vector<sp_nativeinfo_t>& nv)
	{
		sp_nativeinfo_t list[] = {
			{"NavBotUtils.GetEntityWorldSpaceCenter", GetEntityWorldSpaceCenter},
		};

		nv.insert(nv.end(), std::begin(list), std::end(list));
	}
}