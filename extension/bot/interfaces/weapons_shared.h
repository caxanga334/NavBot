#ifndef __NAVBOT_BOT_WEAPONS_SHARED_H_
#define __NAVBOT_BOT_WEAPONS_SHARED_H_

/**
 * @brief Shared bot weapons stuff
 */
namespace botweapons
{
	/**
	 * @brief Attack types for weapons.
	 */
	enum class AttackType : std::int8_t
	{
		PRIMARY = 0,
		SECONDARY,
		TERTIARY,

		MAX_ATTACK_TYPES
	};

	inline bool IsValidAttackType(AttackType type)
	{
		switch (type)
		{
		case AttackType::PRIMARY:
			[[fallthrough]];
		case AttackType::SECONDARY:
			[[fallthrough]];
		case AttackType::TERTIARY:
			return true;
		default:
			return false;
		}
	}

	// This functions guarantees that the returned attack type is valid for use.
	// Prevents out of bound array access.
	inline AttackType GetValidAttackType(AttackType type)
	{
		switch (type)
		{
		case AttackType::PRIMARY:
			[[fallthrough]];
		case AttackType::SECONDARY:
			[[fallthrough]];
		case AttackType::TERTIARY:
			return type;
		default:
			return AttackType::PRIMARY;
		}
	}
}

#endif // !__NAVBOT_BOT_WEAPONS_SHARED_H_
