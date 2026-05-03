#include NAVBOT_PCH_FILE
#include "css_nav_mesh.h"

extern NavAreaVector TheNavAreas;

CCSSNavMesh::CCSSNavMesh() :
	CNavMesh()
{
	m_drawcssattribs = nullptr;
}

CNavArea* CCSSNavMesh::CreateArea(void) const
{
	return new CCSSNavArea(GetNavPlace());
}

uint32_t CCSSNavMesh::GetSubVersionNumber(void) const
{
	/* CSS Version history
	* 1: Initial CSS nav mesh
	*/

	return 1;
}

void CCSSNavMesh::Update()
{
	CNavMesh::Update();

	if (CNavMesh::IsEditing())
	{
		if (IsDrawingCSSAttributes())
		{
			if (extmanager->GetListenServerHost() != nullptr)
			{
				FOR_EACH_VEC(TheNavAreas, it)
				{
					CCSSNavArea* area = static_cast<CCSSNavArea*>(TheNavAreas[it]);
					area->DrawCSSAttributes();
				}
			}
		}
	}
}

bool CCSSNavMesh::IsDrawingCSSAttributes() const
{
	return m_drawcssattribs->GetBool();
}

void CCSSNavMesh::RegisterModCommands()
{
	RegisterCSSNavAreaEditCommands();
	CServerCommandManager& manager = extmanager->GetServerCommandManager();
	m_drawcssattribs = manager.RegisterConVar("sm_css_nav_show_attributes", "Toggles the drawing of CSS attributes.", "0", FCVAR_GAMEDLL | FCVAR_CHEAT);
}

