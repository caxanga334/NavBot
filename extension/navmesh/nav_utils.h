#ifndef NAVMESH_UTILS_H_
#define NAVMESH_UTILS_H_

#include <util/librandom.h>
#include "nav.h"
#include "nav_area.h"

// undef valve mathlib stuff so we can use std version
#undef max
#undef min
#undef clamp

namespace navutils
{
	template <typename F, typename T>
	static void CollectNavAreasInRadius(const Vector& center, const float radius, F shouldCollectFunctor, std::vector<T*>& out)
	{
		extern NavAreaVector TheNavAreas;
		FOR_EACH_VEC(TheNavAreas, it)
		{
			CNavArea* basearea = TheNavAreas[it];

			if ((center - basearea->GetCenter()).Length() <= radius)
			{
				T* area = static_cast<T*>(basearea);
				if (shouldCollectFunctor(area))
				{
					out.push_back(area);
				}
			}
		}
	}
}

#endif // !NAVMESH_UTILS_H_
