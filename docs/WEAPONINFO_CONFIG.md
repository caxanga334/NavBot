# Weapon Information Configuration File

The weapon information provide bots with information about how a weapon can be used.    
The configuration file is stored at `config/<mod folder>/weapons.cfg`.    
It's recommended that you create a copy and rename it to `weapons.custom.cfg` before making modifications to it.

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
|infinite_reserve_ammo|This weapon has infinite reserve ammo.|boolean|
|primary_no_clip|This weapon doesn't uses clips for primary attack.|boolean|
|secondary_no_clip|This weapon doesn't uses clips for secondary attack.|boolean|
|secondary_uses_primary_ammo_type|The secondary attack uses primary ammo type.|boolean|
|low_primary_ammo_threshold|Threshold to consider the bot to be low on ammo for the primary ammo type.|integer|
|low_secondary_ammo_threshold|Threshold to consider the bot to be low on ammo for the secondary ammo type.|integer|
|slot|Which slot this weapon uses.|integer|
|attack_interval|Delay in seconds between attacks. The bot releases the attack buttons between intervals. Negative values to make the bot hold the attack button.|float|
|attack_range_override|Overrides the maximum range between the bot and the target.|float|
|use_secondary_attack_chance|Chance from 1 to 100 to use the secondary attack when both primary and secondary are available.|integer|
|custom_ammo_property_name|Entity property the ammo for this weapon is stored at.|string|
|custom_ammo_property_source|Where is the entity property located. Valid values are "player" and "weapon".|string|
|custom_ammo_property_type|Custom ammo property type. "networked" for networked properties and "datamap" for datamaps.|string|
|custom_ammo_property_out_of_ammo_threshold|If the property value is equal or less than this, the weapon is out of ammo.|float|
|custom_ammo_property_is_float|Is the entity property used for custom ammo a float? If not, it's an integer.|boolean|
|priority_dynamic_has_secondary_ammo|If non zero and the weapon has secondary ammo, add this to the weapon's priority value.|integer|
|priority_dynamic_health_percentage|If the bot health matches the condition, add this to the weapon's priority value|integer|
|priority_dynamic_health_percentage_threshold|If the bot health percentage is equal or less than this, then add `priority_dynamic_health_percentage` to the weapon's priority. Negative value (default) disables this.|float|
|deployed_property_name|Name of a networked property used to check if the weapon is scoped in or deployed. Only supports booleans for now.|string|
|deployed_property_source|Deployed status property source. "player" or "weapon".|string|
|needs_to_be_deployed_to_fire|If enabled, bots will deploy/scope-in before firing with this weapon.|boolean|
|disable_dodge|If enabled, bots won't dodge enemy attacks while using this weapon.|boolean|
|selection_max_range_override|If positive, overrides the maximum range value used in weapon selection.|float|

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
|hold_button_time|How long to keep pressing the attack button in seconds. Negative values for a single tap.|float|
|melee|Is melee attack?|boolean|
|explosive|Is explosive attack?|boolean|

### Weapon's Special Function

The special function section provide bots with information on how to use a weapon special secondary ability, especially those that doesn't use standard attack buttons or ammo storage.    
Example of weapons with such functions (TF2): The Soda Popper, The Hitman's Heatmaker, The Cleaner's Carbine.    
The following key value pairs should be on a `special_function` section on the weapon config file.

|Key Name|Description|Type|
|:---:|:---:|:---:|
|property_name|Name of the networked property to read.|string|
|property_source|Where is the networked property located. "player" or "weapon".|string|
|property_is_float|Is the networked property internally a float. Integer if set to false.|boolean|
|available_threshold|The networked property value must be greater than this to be used.|float|
|button_to_press|Which button the bot should press to activate the special functionl. Options: "secondary_attack", "tertiary_attack", "reload"|string|
|delay_between_uses|Delay in seconds between uses of the special function. Negative values for no delay.|float|
|hold_button_time|How long to press the special function button in seconds. Negative values for a single tap.|float|
|min_range_to_activate|Bots won't use the special function is the distance to the current enemy is less than this value.|float|
|max_range_to_activate|Bots won't use the special function is the distance to the current enemy is greater than this value.|float|

Additional information:

* Bots won't use the weapon's special function if the current enemy is outside the weapon's main attack range.
* Maximum range has a default value of `900000`.

### Special Values and Additional Info

Negative `priority` values marks the weapon as a non combat weapon. This makes the weapon be ignored when selecting the best weapon to engage an enemy.
This is used on weapons that shouldn't be used in combat and should only be used in special occasions, example: The C4 from Counter-Strike.

If `maxrange` is less than *-9000*, the attack function is considered to the disabled.

If `projectilespeed` is negative, the weapon is considered to be a hitscan weapon.

`headshot_range_multiplier` ranges from 0.0 to 1.0 and is used to multiply the attack's `maxrange` value. Example: If a weapon has a `maxrange` of 2048 and a `headshot_range_multiplier` of 0.5, the bots will only aim at the head if the range to the enemy is 1024 or less.

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