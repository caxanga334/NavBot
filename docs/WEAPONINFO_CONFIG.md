# Weapon Information Configuration File

The weapon information provide bots with information about how a weapon can be used.    
The configuration file is stored at `config/navbot/<mod folder>/weapons.cfg`.    
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
|min_required_skill|Minimum weapon skill required to use this weapon.|integer|
|weapon_type|Weapon type. See docs below.|string|
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
|scopein_attack_delay|Delay in seconds before the bot starts firing their weapon after deploying/scoping with their current weapon.|float|
|initial_attack_delay|Delay in seconds before the bot starts firing their weapon when combat start.|float|
|scopein_min_range|Don't use scope/deploy weapon if the distance between the bot and the enemy is less than this.|float|
|attack_range_override|Overrides the maximum range between the bot and the target.|float|
|spam_time|How long in seconds to continue firing the weapon at the enemy LKP.|float|
|use_secondary_attack_chance|Chance from 1 to 100 to use the secondary attack when both primary and secondary are available.|integer|
|setup_custom_ammo_property|Custom ammo Entity Property Setup arguments. See doumentation below.|string|
|is_deployed_property_setup|Is weapon deployed/scoped Entity Property Setup arguments. See doumentation below.|string|
|needs_to_be_deployed_to_fire|If enabled, bots will deploy/scope-in before firing with this weapon.|boolean|
|disable_dodge|If enabled, bots won't dodge enemy attacks while using this weapon.|boolean|
|selection_max_range_override|If positive, overrides the maximum range value used in weapon selection.|float|
|tags|Comma delimited list of weapon tags. Existing tags will be purged.|string list|
|add_tags|Comma delimited list of weapon tags. This version doesn't clear the existing tags.|string list|
|clear_tags|Removes all tags from the weapon.|N/A|
|remove_tags|Comma delimited list of weapon tags to remove from the weapon.|string list|
|preferred_aim_spot| Preferred position to aim the weapon at. Available options are: head, center, origin. |string|
|is_template|This is a template entry for variantof. More information below.|boolean|
|ammo_source_entity|Classname of the ammo entity for this weapon. This is optional.|string|
|can_fire_underwater|If true (default), the weapon can be used underwater.|boolean|

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
|delay_between_attacks|Delay in seconds between attacks (button presses). This delay is per attack.|float|
|range_mapped_attack_delay|Sets the enemy distance mapped attack delay.|string|
|block_attacks_time|Delay in seconds between attacks (button presses). This delay is shared between attack types.|float|
|melee|Is melee attack?|boolean|
|explosive|Is explosive attack?|boolean|

### Weapon's Special Function

The special function section provide bots with information on how to use a weapon special secondary ability, especially those that doesn't use standard attack buttons or ammo storage.    
Example of weapons with such functions (TF2): The Soda Popper, The Hitman's Heatmaker, The Cleaner's Carbine.    
The following key value pairs should be on a `special_function` section on the weapon config file.

|Key Name|Description|Type|
|:---:|:---:|:---:|
|setup_property|Entity Property Setup arguments. See doumentation below.|string|
|button_to_press|Which button the bot should press to activate the special functionl. See the Button List below.|string|
|delay_between_uses|Delay in seconds between uses of the special function. Negative values for no delay.|float|
|hold_button_time|How long to press the special function button in seconds. Negative values for a single tap.|float|
|min_range_to_activate|Bots won't use the special function is the distance to the current enemy is less than this value.|float|
|max_range_to_activate|Bots won't use the special function is the distance to the current enemy is greater than this value.|float|

Additional information:

* Bots won't use the weapon's special function if the current enemy is outside the weapon's main attack range.
* Maximum range has a default value of `900000`.

### Special Values and Additional Info

* If `maxrange` is less than *-9000*, the attack function is considered to the disabled.    
* If `projectilespeed` is negative, the weapon is considered to be a hitscan weapon.    
`headshot_range_multiplier` ranges from 0.0 to 1.0 and is used to multiply the attack's `maxrange` value. Example: If a weapon has a `maxrange` of 2048 and a `headshot_range_multiplier` of 0.5, the bots will only aim at the head if the range to the enemy is 1024 or less.    
* When attacking enemies, bots will only move towards the enemy if they are not visible or if they are outside the range of all weapons owned by the bot.  
* By default, the weapon's maximum range is used as the minimum distance between the bot and the enemy. This distance can be overriden with `attack_range_override`.  
* This can be used to make the bot close the distance between them and the enemy without having to reduce the weapon's maximum range.    
* Bots identify weapons via their entity classname and economy index if available for the mod.     
* This means on games like Team Fortress 2, you can have unique settings for weapons that share the same classname but not economy index like the stock Rocket Launcher and The Liberty Launcher.    
* When searching for weapon information, economy index is searched first, then classname.
* Both `delay_between_attacks` and `block_attacks_time` is a delay in button presses, the former is reset when the used attack type changes (primary, secondary) while the latter does not.

### Using **range_mapped_attack_delay**

The attack function's `range_mapped_attack_delay` property needs four parameters.    
`range_mapped_attack_delay <range min> <range max> <delay min> <delay max>`.    
To disable it, use `range_mapped_attack_delay disable`.    

## Weapon Types

Weapon types provides a basic type based identification of weapons for bots.    
List of valid weapon types:    

* not_useable_weapon : This weapon should not be used (replaces negative priority values).
* combat_weapon : The default type if not specified in the config file, a weapon that can be used to shoot at enemies.
* buff_item : This weapon provides a buff to the user (IE: TF2's crit a cola, soldier's banner).
* defensive_buff_item : This weapon provides a defensive buff to the user. (IE: TF2's bonk).
* heal_item : This weapon can heal the user.
* medical_weapon : This weapon can heal others. (IE: TF2's medigun).
* mobility_tool : This weapon provides additional mobility to the user. (IE: TF2's grappling hook).
* combat_grenade : A grenade used to deal damage to enemies.
* support_grenade : An utility grenade (IE: smoke, flashbangs).

## variantof

To make the config file neater, you can use `"variantof"    "weapon_entry_name"` to inherit another weapon's properties.    
This also reduces the amount of sycronization needed when updating properties shared by both weapons.    
There are a few rules you must obey when using `variantof`:

* The config entry you want the current to be a variant of must come first (be above it).
* Any properties you want to modify/add must be listed **BELOW** the `variantof` KV pair.

### Templates

Entries can be turned into a template by adding the `"is_template"    "true"` property.    
Template entries ignores some error checks and also won't be used by the bots.    

## Dynamic Priorities

Weapons supports condition based dynamic priorities.    
The dynamic priority values are added to the weapon's base priority.    
Dynamic priorities should be added to a `dynamic_priorities` section of the weapon.    

|Priority Key Name|Description|Arguments|
|:---:|:---:|:---:|
|health|Changes the weapon's priority based on the bot's health.|priority value (int), threshold value (float), greater than (boolean)|
|range|Changes the weapon's priority based on the distance between the bot and the current enemy.|priority value (int), threshold value (float), greater than (boolean)|
|secondary_ammo|Changes the weapon's priority if secondary ammo is available for this weapon.|priority value (int),|
|aggression|Changes the weapon's priority based on the bot's difficulty profile aggression value.|priority value (int), threshold value (int), greater than (boolean)|
|classname|Changes the weapon's priority when attacking a specific entity classname. Supports patterns.|priority value (int), classname (string)|
|erase|Special keyworld that instructs the config parser to remove all dynamic priorities from the weapon|None|

### Dynamic Priority Key Format

Dynamic priorities are uses the following format: `"type name" "arguments"`.    
**Arguments** are delimited by comma.

```
// Add 30 weapon priority when secondary ammo is available
"secondary_ammo" "30"
// Add -10 weapon priority when the range to the current enemy is greater than 1000 units.
"range" "-10,1000.0,true"
// Add 50 weapon priority when the bot's health percentage is less than 0.5
"health" "50,0.5,false"
// Removes all dynamic priority entries from this weapon.
"erase" ""
```

## Aim Spots

The `preferred_aim_spot` is an optional property that allows overrinding the default aim position selection for a specific weapon.    
For example, it's used in the TF2 soldier's rocket launcher to make the bots aim at the player's feet.    
If no preferred aim spot is defined, the bot will aim at the first spot with a clear line of fire in the following order: Head (if allowed), center and origin.    
To enable headshots, setting `can_headshot` to **true** is still required! `preferred_aim_spot` is not required for headshots.    

## Button List

Valid values for *button list* attributes.    

|Name|Description|
|:---:|:---:|
| BUTTON_INVALID | Invalid/no button. |
| BUTTON_ATTACKPRIM | Primary attack. |
| BUTTON_ATTACKSEC | Secondary attack. |
| BUTTON_ATTACKSPECIAL | Tertiary/Special attack. |
| BUTTON_JUMP | Jump. |
| BUTTON_CROUCH | Crouch. |
| BUTTON_FORWARDS | Move forward. |
| BUTTON_BACKWARDS | Move backwards. |
| BUTTON_USE | Use. |
| BUTTON_MOVELEFT | Strafe left. |
| BUTTON_MOVERIGHT | Strafe right. |
| BUTTON_MOVEUP | Move up. |
| BUTTON_MOVEDOWN | Move down. |
| BUTTON_RELOAD | Reload. |
| BUTTON_ALT1 | Alt1 |
| BUTTON_RUN | Run |
| BUTTON_SPEED | Speed |
| BUTTON_WALK | Walk |
| BUTTON_MODCUSTOM1 | Custom mod button 1 |
| BUTTON_MODCUSTOM2 | Custom mod button 2 |
| BUTTON_MODCUSTOM3 | Custom mod button 3 |
| BUTTON_MODCUSTOM4 | Custom mod button 4 |

The button names comes from the Source SDK.    
What each button does depends on the game and is common for the button's action to not match their name.    
For example, in Insurgency, the run button is used to change the weapon's fire mode.    

## Entity Property Setup

For attributes that mention it's an entity property setup, you need to provide a few comma delimited arguments to it.    
Format: `Property Name,Property Type,Value Type,Compare Type,Is On Player,value 1,value 2`.    

* Property Name: Name of the entity property.
* Property Type: Type of the property. Can be **Prop_Send** for networked properties or **Prop_Data** for datamaps.
* Value Type: The property's value type: Can be **bool**, **int** or **float**.
* Compare Type: How the value should be compared.
* Is On Player: This is a boolean, true if the property is on the player's entity, false if it's on the weapon. Can be: true, false, yes, no
* Value 1: The value the property will be compared to.
* Value 2: The second value, this is only required if the comparison type requires it.

### Compare Types

|Name|Behavior|Notes| 
|:---:|:---:|:---:|
|equals|Checks if the property value is equal to value 1.|Cannot be used with floats.|
|notequals|Checks if the property value is not equal to value 1.|Cannot be used with floats.|
|greater|Checks if the property value is greater than value 1.|Cannot be used with booleans.|
|less|Checks if the property value is less than value 1.|Cannot be used with booleans.|
|between|Checks if the property value is greater than value 1 and less than value 2.|Cannot be used with booleans. Requires value 2 to be set.|
|bitset|Performs a bitwise AND to determine if the value 1 bit is set on the property's value.|Can only be used with ints.|

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