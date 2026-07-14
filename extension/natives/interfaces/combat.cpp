#include NAVBOT_PCH_FILE
#include <bot/basebot.h>
#include <util/pawnutils.h>
#include "combat.h"

namespace natives::bots::interfaces::combat
{
	static cell_t DisableCombat(IPluginContext* context, const cell_t* params)
	{
		ICombat* iface = pawnutils::UnsafeCastPawnAddressToObject<ICombat>(context, params, 1);
		if (!iface) { context->ReportError("NULL interface ptr!"); return 0; }

		float time = pawnutils::ReadFloat(params, 2);
		iface->DisableCombat(time);
		return 0;
	}
	static cell_t EnableCombat(IPluginContext* context, const cell_t* params)
	{
		ICombat* iface = pawnutils::UnsafeCastPawnAddressToObject<ICombat>(context, params, 1);
		if (!iface) { context->ReportError("NULL interface ptr!"); return 0; }

		iface->EnableCombat();
		return 0;
	}
	static cell_t FireWeaponAt(IPluginContext* context, const cell_t* params)
	{
		ICombat* iface = pawnutils::UnsafeCastPawnAddressToObject<ICombat>(context, params, 1);
		if (!iface) { context->ReportError("NULL interface ptr!"); return 0; }

		CBaseEntity* entity = pawnutils::ReadEntity(context, params, 2);

		if (!entity)
		{
			return 0;
		}

		bool doAim = pawnutils::ReadBool(params, 3);
		iface->FireWeaponAt(entity, doAim);
		return 0;
	}
	void setup(std::vector<sp_nativeinfo_t>& nv)
	{
		sp_nativeinfo_t list[] = {
			{"NavBotCombatInterface.Disable", DisableCombat},
			{"NavBotCombatInterface.Enable", EnableCombat},
			{"NavBotCombatInterface.FireWeaponAt", FireWeaponAt},
		};

		nv.insert(nv.end(), std::begin(list), std::end(list));
	}
}