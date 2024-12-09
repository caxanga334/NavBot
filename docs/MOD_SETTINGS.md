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

| Key | Description | Range |
|:---:|:---:|:---:|
| defend_rate | Used by bots to determine if they should defend or attack. | 0 - 100 |
| stuck_suicide_threshold | If a bot gets this many stuck events on a roll, they will use the kill command. | 5 - 60 |