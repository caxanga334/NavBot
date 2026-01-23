#ifndef __NAVBOT_INSMIC_LIB_H_
#define __NAVBOT_INSMIC_LIB_H_

#include "insmic_shareddefs.h"

namespace insmiclib
{
	inline int EncodeSquadData(int squad, int slot)
	{
		int i = 0;
		int* ptr = &i;
		insmic::InsMICEncodedSquadInfoData_t* data = reinterpret_cast<insmic::InsMICEncodedSquadInfoData_t*>(ptr);

		data->squad = squad + 1;
		data->slot = slot + 1;

		return i;
	}

	inline void DecodeSquadData(int encoded, int& squad, int& slot)
	{
		int* ptr = &encoded;
		insmic::InsMICEncodedSquadInfoData_t* data = reinterpret_cast<insmic::InsMICEncodedSquadInfoData_t*>(ptr);

		squad = data->squad - 1;
		slot = data->slot - 1;
	}
}


#endif // !__NAVBOT_INSMIC_LIB_H_
