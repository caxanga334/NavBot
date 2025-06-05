#ifndef __NAVBOT_BOT_PATH_SHAREDDEFS_H_
#define __NAVBOT_BOT_PATH_SHAREDDEFS_H_

#include <array>
#include <string_view>

namespace AIPath
{
	// Path segment type, tell bots how they should traverse the segment
	enum SegmentType
	{
		SEGMENT_GROUND = 0, // Walking over solid ground
		SEGMENT_DROP_FROM_LEDGE, // Dropping down from a ledge/cliff 
		SEGMENT_CLIMB_UP, // Climbing over an obstacle
		SEGMENT_JUMP_OVER_GAP, // Jumping over a gap/hole on the ground
		SEGMENT_LADDER_UP, // Going up a ladder
		SEGMENT_LADDER_DOWN, // Going down a ladder
		SEGMENT_CLIMB_DOUBLE_JUMP, // Climbing over an obstacle that requires a double jump
		SEGMENT_BLAST_JUMP, // Blast/Rocket jump to the next segment
		SEGMENT_ELEVATOR,// Use an elevator
		SEGMENT_GRAPPLING_HOOK, // Use a grappling hook
		SEGMENT_CATAPULT, // Use a catapult

		MAX_SEGMENT_TYPES
	};

	enum ResultType
	{
		COMPLETE_PATH = 0, // Full path from point A to B
		PARTIAL_PATH, // Partial path, doesn't reach the end goal
		NO_PATH // No path at all
	};

	inline const char* SegmentTypeToString(SegmentType type)
	{
		using namespace std::literals::string_view_literals;

		constexpr std::array names = {
			"GROUND"sv,
			"DROP_FROM_LEDGE"sv,
			"CLIMB_UP"sv,
			"JUMP_OVER_GAP"sv,
			"LADDER_UP"sv,
			"LADDER_DOWN"sv,
			"CLIMB_DOUBLE_JUMP"sv,
			"BLAST_JUMP"sv,
			"ELEVATOR"sv,
			"GRAPPLING_HOOK"sv,
			"CATAPULT"sv,
		};

		static_assert(names.size() == static_cast<size_t>(SegmentType::MAX_SEGMENT_TYPES), "name array and SegmentType enum mismatch!");

		return names[static_cast<std::size_t>(type)].data();
	}
}

#endif // !__NAVBOT_BOT_PATH_SHAREDDEFS_H_
