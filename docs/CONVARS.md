# NavBot's Console Variables

List of console variables for NavBot.

## Bot related Cvars

|ConVar Name|Description|Type|Additional Information|
|:---:|:---:|:---:|:---:|
|sm_navbot_skill_level|The skill group to use when selecting difficulty profiles.|Integer|Must be equals or greater than zero.|
|sm_navbot_debug_disable_gamemode_ai|Disables the bot game mode behavior.|Boolean|Cheat protected.|
|sm_navbot_debug_blind|Disables the bot vision.|Boolean|Cheat protected.|
|sm_navbot_debug_filter|When debugging bots, only show debug information for the bot of this index. Use 0 for all bots.|Integer|Cheat protected.|
|sm_navbot_bot_no_ammo_check|When enabled, bots won't check if they're low on ammo.|Boolean|Enable this on game modes with infinite ammo.|
|sm_navbot_path_segment_draw_limit|Maximum number of path segments draw at once.|Integer|The game client may crash if too many debug overlays are being draw at the same time.|
|sm_navbot_path_max_segments|Maximum number of path segments a path can have.|Integer|Improves CPU performance by limiting the number of segments processed on long paths.|
|sm_navbot_path_goal_tolerance|Default distance tolerance to determine if the bot reached the current path goal.|Float|Default value used when a hard coded value isn't set.|
|sm_navbot_path_skip_ahead_distance|Default distance to look ahead on the path and skip ground segments that can be reached directly.|Float|Default value used when a hard coded value isn't set.|
|sm_navbot_path_obstacle_scan|How frequently to scan for obstacle on the bot's path. Increase to improve CPU performance. Decrease to improve the bot's responsiveness to obstacles.|Float|N/A|
|sm_navbot_path_break_enemy_visible|If enabled, bots will break obstacles on their path while enemies are visible.|Integer|N/A|
|sm_navbot_aim_stability_max_rate|Maximum angle change rate to consider the bot aim to be stable.|Float|N/A|
|sm_navbot_bot_name_prefix|Prefix to add to bot names.|String|N/A|


# Game Specific ConVars

- [Team Fortress 2]
- [Day of Defeat: Source]

<!-- Links -->
[Team Fortress 2]: game_docs/TF2_CONVARS.md
[Day of Defeat: Source]: game_docs/DODS_CONVARS.md