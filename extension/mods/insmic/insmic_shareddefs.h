#ifndef __NAVBOT_INSMIC_SHAREDDEFS_H_
#define __NAVBOT_INSMIC_SHAREDDEFS_H_

namespace insmic
{
	constexpr int UNLIMITED_SUPPLIES = -1;
	constexpr int DISABLED_SQUAD = -1;
	constexpr int MAX_SQUAD_SLOTS = 8;
	constexpr int INVALID_SLOT = -1;
	constexpr int DEFAULT_SLOT = 0;

	enum InsMICTeam
	{
		INSMICTEAM_UNASSINGED = 0,
		INSMICTEAM_USMC,
		INSMICTEAM_INSURGENTS,
		INSMICTEAM_SPECTATOR,
	};

	enum InsMICSquads_t
	{
		INSMIC_INVALID_SQUAD = DISABLED_SQUAD,
		INSMIC_SQUAD_ONE = 0,
		INSMIC_SQUAD_TWO,
		INSMIC_MAX_SQUADS
	};

	struct InsMICEncodedSquadInfoData_t
	{
		unsigned squad : 2;
		unsigned slot : 4;
	};
}

#endif // !__NAVBOT_INSMIC_MOD_SHAREDDEFS_H_

