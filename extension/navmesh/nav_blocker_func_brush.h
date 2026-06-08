#ifndef __NAVBOT_NAV_BLOCKER_FUNC_BRUSH_H_
#define __NAVBOT_NAV_BLOCKER_FUNC_BRUSH_H_

#include "nav_blocker.h"

class CFuncBrushNavBlocker : public CNavBlocker<CNavArea>
{
public:
	CFuncBrushNavBlocker()
	{
		m_blocked = false;
	}

	bool IsValid() const override;
	void Update() override;
	bool IsBlocked(int teamID) const { return m_blocked; }
	bool RemoveOnRecompute() const override { return true; }
	const char* GetName() const override { return "Brush Blocker"; }
	void PrintDebugInfo() const override;

	/**
	 * @brief Initializes the blocker.
	 * @param brush Pointer to a func_brush entity.
	 */
	virtual void Init(CBaseEntity* brush);

protected:
	void SetBlockedStatus(bool status) { m_blocked = status; }

private:
	bool m_blocked;
	CHandle<CBaseEntity> m_brush; // the func_brush entity
};

#endif // !__NAVBOT_NAV_BLOCKER_FUNC_BRUSH_H_
