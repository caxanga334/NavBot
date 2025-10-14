<!-- Documentation for Nav Places editing -->

# Nav Places Names (Callouts)

NavBot supports the [nav mesh place naming](https://developer.valvesoftware.com/wiki/Nav_Mesh_Editing#Place_names_(callouts)), with some new features added to it.    
When place names are available, bots will use them to report an enemy position to humans players.    

## Place Name Databases

NavBot's upgraded the place system to support loading from multiple files.    
The main feature is the addition of map specific place name databases.    
There are 3 database files:

* `data/navbot/nav_places.cfg` (Global)
* `data/navbot/[mod]/nav_places.cfg` (Mod Global)
* `data/navbot/[mod]/[map]_places.cfg` (Per map file)

The global and mod global files needs to be syncronized for everyone for them to work. Because of this, custom place names should **ALWAYS** be added to the per map database file.    
If you have suggestions for the global or mod global place databases, open a pull request!

## Creating Custom Place Names

Create a new text file at `data/navbot/[mod]/[map]_places.cfg`, where `[mod]` is the current mod gamefolder name and `[map]` is the current map name report by the command `sm_nav_print_map_name`.    
The file needs to follow this format:

```
NavPlaces
{
	"PlaceKey1"          "PlaceHumanReadableName1"
	"PlaceKey2"          "PlaceHumanReadableName2"
	...
	"PlaceKeyN"          "PlaceHumanReadableNameN"
}
```

`PlaceKeyN` is the place key name, this is the internal place name, used by the `sm_nav_use_place` command.    
`PlaceHumanReadableNameN` is the human readable name that bots will use when sending chat messages.    
The database file is parsed when the map is loaded, to reload it, reload the map.    
Example:    

```
NavPlaces
{
	/* Custom names */
	"2fortbridgeroof"		"Bridge Roof"
	"2fortrsewers"			"RED Sewers"
	"2fortbsewers"			"BLU Sewers"
}
```

### Tips

The map specific place database is allowed to override an existing place entry display name.    
Just define a place using the same key name of an existing place entry.    
Example:    

```
NavPlaces
{
	/* Overrides */
	/* Overrides the "Middle" display name to "Middle of The Map" */
	"Middle"		"Middle of The Map"
}
```
