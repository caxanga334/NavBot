#include NAVBOT_PCH_FILE
#include <mods/basemod.h>
#include <mods/modhelpers.h>
#include <entities/baseentity.h>
#include "nav_mesh.h"
#include "nav_blocker_func_brush.h"

bool CFuncBrushNavBlocker::IsValid() const
{
	if (m_brush.Get() == nullptr)
	{
		return false;
	}

	return true;
}

void CFuncBrushNavBlocker::Update()
{
	// this is always valid since IsValid is called before Update
	CBaseEntity* brush = m_brush.Get();
	int solidflags = reinterpret_cast<IServerEntity*>(brush)->GetCollideable()->GetSolidFlags();

	if ((solidflags & static_cast<int>(SolidFlags_t::FSOLID_NOT_SOLID)) != 0)
	{
		SetBlockedStatus(false);
	}
	else
	{
		SetBlockedStatus(true);
	}
}

void CFuncBrushNavBlocker::PrintDebugInfo() const
{
	CBaseEntity* brush = m_brush.Get();
	META_CONPRINTF("Entity: %s \n", UtilHelpers::textformat::FormatEntity(brush));

	if (brush)
	{
		META_CONPRINTF("Pos: %s \n", UtilHelpers::textformat::FormatVector(UtilHelpers::getWorldSpaceCenter(brush)));
	}
}

void CFuncBrushNavBlocker::Init(CBaseEntity* brush)
{
	entities::HFuncBrush entity(brush);
	auto solid = entity.GetSolidity();
	
	// always solid brushes are solid for mesh generation and never solids will never block, don't bother with these types.
	if (solid == entities::HFuncBrush::BrushSolidities_e::BRUSHSOLID_NEVER || solid == entities::HFuncBrush::BrushSolidities_e::BRUSHSOLID_ALWAYS)
	{
		return;
	}

	AddAreasTouchingEntity(brush, navgenparams->step_height, navgenparams->half_human_width);

	if (IsAreaVectorEmpty()) { return; }

	m_brush = brush;
}