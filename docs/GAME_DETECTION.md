# Game/Mod Detection

NavBot's game/mod detection is defined in the `configs/navbot/mod_loader.cfg` config file.    
Rename to `mod_loader.custom.cfg` before making modifications to it so changes aren't lost during an update.    

## Format

```
"ModLoader"
{
	"config_entry_name"
	{
		"Description"
		{
			"Name"		"My Source Mod"
			"ID"		"0"
		}
		"Detection"
		{
			"Type"		"GameFolder"
			"Data"      "mysourcemod"
		}
	}
}
```

## Key Values and Sections

The first section after `ModLoader` is the config entry name. This can be anything but should be unique.    
Inside a config entry can be three sections: `Description, Detection and Config`.    

### Description Section

Description contains the mod name and ID.    
The ID is must match one of the hard coded IDs from `ModType` enum.    
See the [gamemods_shared.h](https://github.com/caxanga334/NavBot/blob/main/extension/mods/gamemods_shared.h) file.    
The ID is used to allocate the mod class, so it's very important to use the correct one for the mod.    
Unsupported game/mods should use the base id (0).    
If the mod is a mod of another game (IE: A TF2 mod), use the ID of the base game.    
For example, a TF2 mod like TF2C would use the TF2 ID unless the mod in question has a dedicated ID to it.    

```
"Description"
{
    "Name"		"My Source Mod"
    "ID"		"0"
}
```

### Detection Section

The detection section is how NavBot will detect the current game/mod.    
There are 4 detection methods:    

- GameFolder
- NetProp
- ServerClass
- AppID

```
"Detection"
{
    "Type"		"AppID"
    // Detection data string. Meaning depends on the type.
    "Data"      "foobar"
    // ServerClass name. Used in NetProp and ServerClass detections.
    "PropClass" "CModPlayer"
    // Networked property variable name. Used in NetProp detection type.
    "PropName"  "m_iMyModVar"
    // Steam App ID, used in AppID detection type.
    "AppID"		"440"
}
```

### "GameFolder"

Detects a mod by matching the game folder name.    
Requires the `Data` key. The value should be the game folder name to compare.    

### "NetProp"

Detects by checking if a networked property exists.    
Requires `PropClass` and `PropName` keys.    
`PropClass` is the **ServerClass** name and `PropName` the property name.    

### ServerClass

Detects by checking if a **ServerClass** with the given name exists.    
Requires the `PropClass` key.    
`PropClass` is the **ServerClass** name.    

### AppID

Detects by reading the AppID from *steam.inf* file.    
Requires the `AppID` key.    

### Config section

The configuration section configures the mod interface.    
Currently the only configuration available is an optional game folder name override.    

```
"Config"
{
    // override the game folder to be "tf".
    // this is used when parsing config file and loading nav mesh files.
    "Mod_Folder"    "tf"
    // Fallback folder for loading game specific files.
    // If a file cannot be found on the main mod folder, search this one.
    "Mod_Fallback_Folder" "foo"
}
```