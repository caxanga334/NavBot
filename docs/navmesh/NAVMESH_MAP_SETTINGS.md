# Nav Mesh Map Settings

The map settings is a SMC format configuration file located in `data/navbot/[game folder]/`.    
The file should be named `[map name]_settings.cfg`.    

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
}
```

