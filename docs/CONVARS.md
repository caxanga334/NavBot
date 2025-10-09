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
|sm_navbot_path_useable_scan|How frequently bots scans for useable obstacles on the path. (IE: Doors)|Float|Lower values improves the bot reaction time to obstacles, higher values improves performance.|
|sm_navbot_aim_stability_max_rate|Maximum angle change rate to consider the bot aim to be stable.|Float|N/A|
|sm_navbot_quota_mode|Bot quota mode.|String|'normal' to keep N number of bots in the game. 'fill' to fill until there area N players on the server, bots will leave as human players join in this mode.|
|sm_navbot_quota_quantity|The target number of bots.|Integer|N/A|
|sm_navbot_bot_name_prefix|Prefix to add to bot names.|String|N/A|
|sm_navbot_hide_bots|If enabled, bots are hidden from the server browser.|Boolean|Needs to be enable before adding bots to take effect. Only available on supported engine branches.|
|sm_navbot_allow_hibernation|If enabled, bots will be kicked when the server is empty to allow hibernation.|Boolean|Some games automatically kicks bots to enter hibernation (IE: TF2), NavBot doesn't prevent that.|

## Team Fortress 2 Bot specific ConVars

|ConVar Name|Description|Type|Additional Information|
|:---:|:---:|:---:|:---:|
|sm_navbot_tf_allow_class_changes|If disabled, bots won't change classes.|Boolean|Useful on game modes/maps that forces a specific class.|
|sm_navbot_tf_teammates_are_enemies|If enabled, bots will attack teammates.|Boolean|Useful on deathmatch game modes.|
|sm_navbot_tf_force_class|Forces bots to select this class.|String|Must be a valid class name: scout, soldier, pyro, demoman, heavyweapons, engineer, medic, sniper, spies.|
|sm_navbot_tf_force_gamemode|Force a specific game mode.|Integer|Set to a negative value to use the automatic game mode detection.|
|sm_navbot_tf_mvm_collect_currency|If enabled, bots will collect currency in MvM.|Boolean|A third-party plugin is required to patch the game in order to allow RED bots to collect currency.|
|sm_navbot_tf_debug_bot_upgrades|If enabled, prints MvM upgrades debug messages.|Boolean|Cheat protected.|
|sm_navbot_tf_mod_debug|If enabled, prints some TF2 related debug messages.|Boolean|Cheat protected.|

Values for `sm_navbot_tf_force_gamemode`.

* 0: None
* 1: Capture The Flag
* 2: Control Points
* 3: Attack/Defense Control Points
* 4: King of The Hill
* 5: Mann vs Machine
* 6: Payload
* 7: Payload Race
* 8: Player Destruction
* 9: Robot Destruction
* 10: Special Delivery
* 11: Territorial Control
* 12: Arena
* 13: Passtime
* 14: Versus Saxton Hale (Vscript)
* 15: Zombie Infection (Vscript)
* 16: Gun Game
* 17: Deathmatch
* 18: Slender Fortress
* 19: Lizard Of Oz's FFA Deathmatch (Vscript)

### Notes For TF2 Game Modes

`None` is deathmatch with class behaviors. (IE: medics will heal players, engineers will build.)    
`Deathmatch` is deathmatch without class behaviors. Engineers, medics, snipers and spies won't use their class behavior.    
The following game modes don't have behavior yet: Slender Fortress.    