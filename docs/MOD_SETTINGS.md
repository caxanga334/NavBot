# NavBot Mod Settings File

The mod setting files is used to load some configuration options for NavBot.

## File Parsing

The mod setting values are reset to default on every level change, then the config files are parsed.    
The files are parsed in the following order:    

1. `configs/navbot/settings.cfg` or `configs/navbot/settings.custom.cfg`
2. `configs/navbot/[mod folder]/settings.cfg` or `configs/navbot/[mod folder]/settings.custom.cfg`
3. `configs/navbot/[mod folder]/maps/settings.[map name].cfg`

The parser will parse multiple files if they're present.    
`[map name]` should match the name given by the `sm_nav_print_map_name` command.    
Prefer using the `.custom` suffix.    

## File Format

```
ModSettings
{
	"settings1_key"          "settings1_value"
    "settings2_key"          "settings2_value"
    "settings3_key"          "settings3_value"
    ...
    "settingsN_key"          "settingsN_value"
}
```

## Base Settings

These settings are used by all mods.

| Key | Description | Range | Default |
|:---:|:---:|:---:|:---:|
| update_rate | How frequently in seconds the bot internal state is updated. Lower values improves the bot responsiveness, higher values will decrease CPU usage. | 0.0 - 0.5 | 0.1 |
| vision_npc_update_rate | How frequently in seconds the bot will scan for visible non player entities. Lower values improves the bot responsiveness to detecting NPC entities, higher values will decreate CPU usage. This value is capped by the primary `update_rate` value. | 0.0 - 2.0 | 0.250 |
| vision_statistics_update | How frequently to update the number of visible enemies and allies. | 0.05 - 2.0 | 0.5 |
| inventory_update_rate | How frequently in seconds the bot will update the weapon inventory. Lower values allows the bots to detect new weapons faster, higher values will decrease CPU usage. | 0.1 - 120.0 | 60.0 |
| defend_rate | Used by bots to determine if they should defend or attack. | 0 - 100 | 17 |
| stuck_suicide_threshold | If a bot gets this many stuck events on a roll, they will use the kill command. | 5 - 60 | 15 |
| collect_item_max_distance | Maximum travel distance when collecting items (health, armor, weapons, ammo, ...) | 2048 - 16384 | 5000 |
| max_defend_distance | Maximum distance between objectives and defend flagged waypoints. | 1024 - MAX_COORD | 4096 |
| max_sniper_distance | Maximum distance between objectives and sniper flagged waypoints. | 1024 - MAX_COORD | 8192 |
| rogue_chance | Chance for a bot to use the rogue behavior. Rogue bots ignore map objectives and will roam randomly looking for enemies. | 0 - 100 | 8 |
| rogue_max_time | Maximum time to stay in the rogue behavior. | 90 - 1200 | 300 |
| rogue_min_time | Minimum time to stay in the rogue behavior. | 30 - 600 | 120 |
| movement_break_assist | If enabled, assist the bot with breaking obstacles in their path. | true or false | true |
| movement_jump_assist | If enabled, assist the bot with jumping over obstacles. | true or false | true |
| unstuck_cheats | If enabled, allow bots to cheat to get unstuck. | true or false | true |
| allow_class_changes | If enabled, allow bots to change classes on class based games. | true or false | true |
| unstuck_teleport_threshold | If a bot get this many stuck events on a roll, the bot will teleport. Negative numbers to disable, requres unstuck_cheats. | -1 - 60 | 12 |
| class_change_min_time | Minimum time in seconds before bots may try to change classes. | 30 - 300 | 60 |
| class_change_max_time | Maximum time in seconds before bots may try to change classes. | 60 - 1200 | 180 |
| class_change_chance | Chance that a bot will actually change their class. | 0 - 100 | 75 |
| camp_time_min | Minimum time in seconds a bot will camp. | 15 - 300 | 15 |
| camp_time_max | Maximum time in seconds a bot will camp. | 30 - 1200 | 90 |


### Macro Values

Some setting options uses macro values instead of fixed values.    
These generally depends on the SDK branch NavBot was compiled for.    

* `MAX_COORD`: Maximum world size in hammer units. Generally it's 16384.


## Game Specific Docs

- [DoD:S Mod Settings]
- [TF2 Mod Settings]

<!-- Links -->
[DoD:S Mod Settings]: game_docs/DODS_MOD_SETTINGS.md
[TF2 Mod Settings]: game_docs/TF2_MOD_SETTINGS.md