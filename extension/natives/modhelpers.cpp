#include NAVBOT_PCH_FILE
#include <util/pawnutils.h>
#include <mods/basemod.h>
#include <mods/modhelpers.h>
#include "modhelpers.h"

namespace natives::imodhelpers
{
	static cell_t IsEntityBreakable(IPluginContext* context, const cell_t* params)
	{
		CBaseEntity* entity = pawnutils::ReadEntity(context, params, 1);

		if (!entity) { return 0; }

		return pawnutils::ReturnBool(modhelpers->IsEntityBreakable(entity));
	}
	static cell_t IsEntityDamageable(IPluginContext* context, const cell_t* params)
	{
		CBaseEntity* entity = pawnutils::ReadEntity(context, params, 1);

		if (!entity) { return 0; }

		return pawnutils::ReturnBool(modhelpers->IsEntityDamageable(entity, static_cast<int>(params[2])));
	}
	static cell_t IsEntityDamageableBy(IPluginContext* context, const cell_t* params)
	{
		CBaseEntity* entity = pawnutils::ReadEntity(context, params, 1);

		if (!entity) { return 0; }

		CBaseEntity* attacker = pawnutils::ReadEntity(context, params, 2);

		if (!attacker) { return 0; }

		return pawnutils::ReturnBool(modhelpers->IsEntityDamageableBy(entity, attacker));
	}
	void setup(std::vector<sp_nativeinfo_t>& nv)
	{
		sp_nativeinfo_t list[] = {
			{ "NavBotModHelpers.IsEntityBreakable", IsEntityBreakable },
			{ "NavBotModHelpers.IsEntityDamageable", IsEntityDamageable },
			{ "NavBotModHelpers.IsEntityDamageableBy", IsEntityDamageableBy },
		};

		nv.insert(nv.end(), std::begin(list), std::end(list));
	}
}