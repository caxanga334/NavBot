# Bot Difficulty Profiles

The bot difficulty profile stores difficulty/skill level related variables.    
The ConVar `sm_navbot_skill_level` determines which skill group to use.
These profiles are stored in the `addons/sourcemod/configs/navbot/` folder.    

## Loading Files

The parser supports mod specific profiles and custom overrides.    
Per mod files are stored in the mod folder. Example: `addons/sourcemod/configs/navbot/tf/bot_difficulty.cfg`.    
If that file doesn't exists, it loads the global configuration file located at: `addons/sourcemod/configs/navbot/bot_difficulty.cfg`.

### Overrides

You can override the bot profile files by renaming it to `bot_difficulty.custom.cfg`.    
This allows you to customize the bot profiles without worrying about losing them when updating the extension.    

## Random Difficulty

If `sm_navbot_skill_level` is set to a negative value, a random difficulty profile is generated for each bot instead of selecting a profile from the configuration file.

# Profile Format

Example profile

```
"Easy"
{
    "skill_level"				"0"
    "game_awareness"			"0"
    "aimspeed"					"120"
    "fov"						"60"
    "maxvisionrange"			"1024"
    "maxhearingrange"			"350"
    "minrecognitiontime"		"1.0"
    "predict_projectiles"		"no"
    "allow_headshots"			"no"
    "allow_dodging"				"no"
    "aim_tracking_interval"		"1.0"
    "aggressiveness"			"0"
    "teamwork"					"0"
    "ability_use_interval"		"1.0"
}
```

| Key | Description |
|:---:|:---:|
| skill_level | Which skill level this profile part of. You can have multiple profiles with the same skill level. A random profile will be selected for the bot. |
| game_awareness | The bot game awareness skill. Ranges from 0 to 100. |
| aimspeed | The bot aim turn rate in degrees per second. (AKA sensitivity) |
| fov | The bot field of view in degrees. |
| maxvisionrange | The bot maximum vision range in hammer units. |
| maxhearingrange | The bot maximum hearing range in hammer units. |
| minrecognitiontime | The minimum amount of time in seconds an entity must be visible for the bot to it be recognized by the bot's systems. (Reaction time) |
| predict_projectiles | Determines if the bot is allowed to use prediction when aiming with projectile weapons. |
| allow_headshots | Determines if the bot is allowed to go for headshots. |
| allow_dodging | Determines if the bot is allowed to dodge enemy attacks, making them harded to hit. |
| aim_tracking_interval | How frequently the bot will update the target position when aiming at an entity. |
| aggressiveness | How aggressive this bot is. |
| teamwork | How likely this bot will cooperate with teammates. |
| ability_use_interval | How ofter the bot will check if they can use secondary abilities. (Example: TF2 airblast, demoman charge) |

