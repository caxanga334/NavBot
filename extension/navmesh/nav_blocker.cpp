#include NAVBOT_PCH_FILE
#include "nav_mesh.h"
#include "nav_blocker.h"

void INavBlocker::Register()
{
	TheNavMesh->RegisterNavBlocker(this);
}

void INavBlocker::Unregister()
{
	TheNavMesh->UnregisterNavBlocker(this);
}

void INavBlocker::NotifyDestruction()
{
	INavBlocker::NotifyBlockerDestruction<CNavArea> functor{ this };
	CNavMesh::ForAllAreas<decltype(functor)>(functor);
}

#ifdef EXT_DEBUG

class CDebugBlocker : public CNavBlocker<CNavArea>
{
public:
	CDebugBlocker(const Vector& origin)
	{
		Extent extent;
		extent.lo = (origin + Vector(-512.0f, -512.0f, -256.0f));
		extent.hi = (origin + Vector(512.0f, 512.0f, 256.0f));

		TheNavMesh->CollectAreasOverlappingExtent(extent, m_areas);
		m_endtime = gpGlobals->curtime + 30.0f;
		META_CONPRINTF("%3.2f : CONSTRUCTOR CDebugBlocker %p\n", gpGlobals->curtime, this);
	}

	~CDebugBlocker() override
	{
		META_CONPRINTF("%3.2f : DESTRUCTOR CDebugBlocker %p\n", gpGlobals->curtime, this);
	}

	bool IsValid() override
	{
		META_CONPRINTF("%3.2f : ISVALID CDebugBlocker %p\n", gpGlobals->curtime, this);

		if (gpGlobals->curtime >= m_endtime)
		{
			return false;
		}

		return true;
	}

	void Update() override
	{
		META_CONPRINTF("%3.2f : UPDATE CDebugBlocker %p\n", gpGlobals->curtime, this);
	}

private:
	float m_endtime;
};

CON_COMMAND(sm_nav_debug_create_blocker, "Creates a dynamic nav blocker at your position")
{
	CBaseExtPlayer* host = extmanager->GetListenServerHost();

	if (host)
	{
		CDebugBlocker* blocker = new CDebugBlocker(host->GetAbsOrigin());
		blocker->Register();
	}
}

#endif // EXT_DEBUG
