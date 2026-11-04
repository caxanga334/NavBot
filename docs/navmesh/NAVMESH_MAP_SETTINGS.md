# Nav Mesh Map Settings

The map settings is a SMC format configuration file located in `data/navbot/[game folder]/`.    
The file should be named `[map name]_settings.cfg`.    

# Additional Door Classnames

Some maps adds custom classnames to door entities using "AddOutput classname".    
NavBot's nav mesh won't recognize these door entities due to them having a non default classname.    
Use the `AdditionalDoorClassnames` section to tell the nav mesh that these classnames are also door entities.    

# Useable Obstacles

The `UseableObstacles` sections allows for configuring obstacle/button entity pairs.    
When a bot encounters the obstacle in their path, they will look up for any entities listed in this section, if found, the entity is considered as an useable obstacle.    
Example: This can be used to tell bots they need to press a button to open a door.    
**Important Note**: Bots will walk in a straight line to the button, if this is not enough to reach it, you have to use a prerequisite instead.    

## Entity Info Pair Options

It requires two sections, `ObstacleEntityInfo` (the entity that will be in the bot's path) and `UseableEntityInfo` (the entity the bot needs to +USE).    
Available key values for `ObstacleEntityInfo` and `UseableEntityInfo`.    

| Name | Description |
|:---:|:---:|
| debugname | Name used when reporting errors. Optional but helps finding broken entity links. |
| classname | Entity classname to search. Required unless Hammer ID was used. |
| targetname | Designer given name to the entity. Should always be set if the entity has one. |
| position | Position filter, optional if the entity's targetname is unique, else it's required. The closest entity found to the position is used. |
| hammerid | Searches for an entity by Hammer ID. Hammer ID has priority and classname search won't be used if set. |

## Format

```
NavMapSettings
{
	// List of classnames to be considered as doors, since some level designers changes the door classname using "AddOutput classname".
	// Used by the nav mesh to create auto door blockers.
	"AdditionalDoorClassnames"
	{
		// one classname per key value pair, the value is not used and can be left empty.
		"foobar" ""
	}
	// Instruct bots these entities can be used to clear the path.
	// IE: press a button to open a door
	"UseableObstacles"
	{
		// Adds a new useable obstacle entity pair
		"EntityInfoPair"
		{
			// The useable obstacle
			"ObstacleEntityInfo"
			{
				"classname" "func_door"
				"targetname" "foo_door_1"
				"position" "-642 755 -412"
			}
			"UseableEntityInfo"
			{
				"classname" "func_button"
				"position" "-600 730 -378"
			}
		}
	}
}
```

