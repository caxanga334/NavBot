#ifndef __NAVBOT_PAWN_UTILS_H_
#define __NAVBOT_PAWN_UTILS_H_

#include <ISourceMod.h>

/* SourceMod Utility functions */

namespace pawnutils
{
	inline Vector PawnFloatArrayToVector(cell_t* array)
	{
		return { sp_ctof(array[0]), sp_ctof(array[1]), sp_ctof(array[2]) };
	}
	inline void VectorToPawnFloatArray(cell_t* array, const Vector& vec)
	{
		array[0] = sp_ftoc(vec.x);
		array[1] = sp_ftoc(vec.y);
		array[2] = sp_ftoc(vec.z);
	}
}


#endif // !__NAVBOT_PAWN_UTILS_H_
