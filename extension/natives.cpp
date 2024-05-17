#include <extension.h>
#include "manager.h"
#include "natives.h"

namespace natives
{
	//const sp_nativeinfo_t MyNatives[] =
	//{
	//	{"IsNavBot",	natives::IsNavBot},
	//	{nullptr,			nullptr},
	//};

	void setup(std::vector<sp_nativeinfo_t>& nv)
	{
		sp_nativeinfo_t list[] = {
			{"IsNavBot", IsNavBot},
		};

		nv.insert(nv.end(), std::begin(list), std::end(list));
	}

	cell_t IsNavBot(IPluginContext* context, const cell_t* params)
	{
		int client = params[1];

		if (client < 0 || client > gpGlobals->maxClients)
		{
			return context->ThrowNativeError("Invalid client index %i!", client);
		}

		bool isbot = extmanager->IsNavBot(client);

		return isbot ? 1 : 0;
	}
}