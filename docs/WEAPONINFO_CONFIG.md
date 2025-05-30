# Weapon Information Configuration File

The weapon information provide bots with information about how a weapon can be used.

The configuration file is stored at `config/<mod folder>/weapons.cfg`.

## File Format

The config file uses the SMC format.

### Available Keys

This is a list of keys available for all mods.

|Key Name|Description|Type|
|:---:|:---:|:---:|
|classname|The weapon's entity classname.|string|
|itemindex|The weapon's econ index for games that have it (IE: TF2).|integer|
|priority|Weapon selection priority for this weapon.|integer|
|can_headshot|Can this weapon headshot.|boolean|
|headshot_range_multiplier|Headshot range multiplier.|float|
|headshot_aim_offset|Offset to apply when aiming at the head with this weapon.|Vector|
|infinite_ammo|Does this weapon have infinite reserve ammo.|boolean|
|maxclip1|Maximum ammo in clip 1.|integer|
|maxclip2|Maximum ammo in clip 2.|integer|
|low_primary_ammo_threshold|Threshold to consider the bot to be low on ammo for the primary ammo type.|integer|
|low_secondary_ammo_threshold|Threshold to consider the bot to be low on ammo for the secondary ammo type.|integer|
|slot|Which slot this weapon uses.|integer|
|semiauto|Is the weapon semi automatic (requires the attack button to be released to fire again).|boolean|
|attack_range_override|Overrides the maximum range between the bot and the target.|float|

The following keys applies to attack info sections (`primary_attack_info`, `secondary_attack_info` and `tertiary_attack_info`).

|Key Name|Description|Type|
|:---:|:---:|:---:|
|maxrange|Maximum attack range.|float|
|minrange|Minimum attack range.|float|
|projectilespeed|The initial speed of a projectile fired by this weapon.|float|
|gravity|Gravity effect multiplier on the projectile fired by this weapon.|float|
|ballistic_elevation_min|Elevation value to use for gravity compensation when aiming at the minimum range.|float|
|ballistic_elevation_max|Elevation value to use for gravity compensation when aiming at the maximum range.|float|
|ballistic_elevation_range_start|Range to apply the minimum elevation value.|float|
|ballistic_elevation_range_end|Range to apply the maximum elevation value.|float|
|melee|Is melee attack?|boolean|
|explosive|Is explosive attack?|boolean|

### Special Values and Additional Info

Negative `priority` values marks the weapon as a non combat weapon. This makes the weapon be ignored when selecting the best weapon to engage an enemy.
This is used on weapons that shouldn't be used in combat and should only be used in special occasions, example: The C4 from Counter-Strike.

If `maxrange` is less than *-9000*, the attack function is considered to the disabled.

If `projectilespeed` is negative, the weapon is considered to be a hitscan weapon.

`headshot_range_multiplier` ranges from 0.0 to 1.0 and is used to multiply the attack's `maxrange` value. Example: If a weapon has a `maxrange` of 2048 and a `headshot_range_multiplier` of 0.5, the bots will only aim at the head if the range to the enemy is 1024 or less.

Setting `maxclip1` or `maxclip2` to `-2` flags the weapon as *Clip is reserve ammo*. (The weapon doesn't uses clips and the reserve ammo is used directly, IE: HL2's SMG1 grenades).

When attacking enemies, bots will only move towards the enemy if they are not visible or if they are outside the range of all weapons owned by the bot.  
By default, the weapon's maximum range is used as the minimum distance between the bot and the enemy. This distance can be overriden with `attack_range_override`.  
This can be used to make the bot close the distance between them and the enemy without having to reduce the weapon's maximum range.

Bots identify weapons via their entity classname and economy index if available for the mod.

This means on games like Team Fortress 2, you can have unique settings for weapons that share the same classname but not economy index like the stock Rocket Launcher and The Liberty Launcher.

When searching for weapon information, economy index is searched first, then classname.

## Console Commands

The weapon configuration file can be reloaded with the `sm_navbot_reload_weaponinfo_config` command.

## Example

```
WeaponInfoConfig
{
	"crowbar"
	{
		"classname" "weapon_crowbar"
		"priority" "10"
		"slot" "1"

		"primary_attack_info"
		{
			"maxrange" "96"
			"minrange" "-1"
			"melee" "true"
		}
	}
	"glock"
	{
		"classname" "weapon_glock"
		"priority" "20"
		"slot" "2"
		"can_headshot" "true"
		"headshot_range_multiplier" "0.25"
		"maxclip1" "17"
		"low_primary_ammo_threshold" "30"

		"primary_attack_info"
		{
			"maxrange" "3000"
			"minrange" "-1"
		}
		"secondary_attack_info"
		{
			"maxrange" "400"
			"minrange" "-1"
		}
	}
}
```