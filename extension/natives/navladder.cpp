#include NAVBOT_PCH_FILE
#include <util/pawnutils.h>
#include <navmesh/nav_mesh.h>
#include <navmesh/nav_ladder.h>
#include "navladder.h"

namespace natives::navladder
{
	static cell_t GetLength(IPluginContext* context, const cell_t* params)
	{
		CNavLadder* ladder = pawnutils::UnsafeCastPawnAddressToObject<CNavLadder>(context, params, 1);

		if (!ladder)
		{
			context->ReportError("NULL nav ladder!");
			return 0;
		}

		return pawnutils::ReturnFloat(ladder->m_length);
	}

	void setup(std::vector<sp_nativeinfo_t>& nv)
	{
		sp_nativeinfo_t list[] = {
			{"NavBotNavLadder.GetLength", GetLength},
		};

		nv.insert(nv.end(), std::begin(list), std::end(list));
	}
}