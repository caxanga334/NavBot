# NavBot Mod Settings File

The mod setting files is used to load some configuration options for NavBot.

## Load Priority

The first file found is parsed.    
The load order is:

1. `configs/navbot/[mod folder]/maps/settings.[map name].cfg`
2. `configs/navbot/[mod folder]/settings.custom.cfg`
3. `configs/navbot/[mod folder]/settings.cfg`
4. `configs/navbot/settings.custom.cfg`
5. `configs/navbot/settings.cfg`

`[map name]` should match the name given by the `sm_nav_print_map_name` command.

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

## Team Fortress 2 Settings

These settings are only used in Team Fortress 2.

| Key | Description | Range | Default |
|:---:|:---:|:---:|:---:|
| engineer_nest_dispenser_range | Maximum distance between the sentry gun and the dispenser. | 600 - 4096 | 900 |
| engineer_nest_exit_range | Maximum distance between the sentry gun and teleporter exit. | 600 - 4096 | 1200 |
| entrance_spawn_range | Maximum distance between the telepoter entrance and the active team spawn point. | 1500 - 6000 | 2048 |
| mvm_sentry_to_bomb_range | Maximum distance to search for spots to build a sentry gun in MvM. | 1000 - 3000 | 1500 |
| engineer_destroy_travel_range | When moving buildings, if the travel distance to reach the building is larger than this, destroy it instead. | 1000 - 10000 | 4500 |
| engineer_move_check_interval | How frequently in seconds engineers check if they should move their buildings. | 30 - 180 | 60 |
| engineer_sentry_killassist_threshold | If a sentry hasn't got at least this many kills plus assists between two move checks, bots will move them. | 1 - 30 | 5 |
| engineer_teleporter_uses_threshold | If a teleporter wasn't used at least this many times between two move checks, bots will move them. | 1 - 10 | 4 |
| engineer_help_ally_max_range | Ally building maximum search range. | 500 - 4000 | 1500 |
| engineer_nav_build_range | Maximum search distance for a random nav area to build. | 1024 - 10000 | 2048 |
| engineer_nav_build_check_vis | If enabled, engineers will only build on random areas that can be seen from the map's objective. | true or false | false |
| engineer_trust_waypoints | If enabled, engineers won't move their sentry gun if the current waypoint being used is still valid. | true or false | true |

### Macro Values

Some setting options uses macro values instead of fixed values.    
These generally depends on the SDK branch NavBot was compiled for.    

* `MAX_COORD`: Maximum world size in hammer units. Generally it's 16384.