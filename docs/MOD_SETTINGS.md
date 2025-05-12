# NavBot Mod Settings File

Mod settings are loaded from `navbot/**GAME FOLDER**/configs/settings.cfg`.

These are a collection of shared and mod specific settings used by the extension.

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
| inventory_update_rate | How frequently in seconds the bot will update the weapon inventory. Lower values allows the bots to detect new weapons faster, higher values will decrease CPU usage. | 0.1 - 60.0 | 1.0 |
| defend_rate | Used by bots to determine if they should defend or attack. | 0 - 100 | 33 |
| stuck_suicide_threshold | If a bot gets this many stuck events on a roll, they will use the kill command. | 5 - 60 | 15 |
| collect_item_max_distance | Maximum travel distance when collecting items (health, armor, weapons, ammo, ...) | 2048 - 16384 | 5000 |