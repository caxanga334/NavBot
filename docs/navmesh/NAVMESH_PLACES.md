<!-- Documentation for Nav Places editing -->

# Nav Places

Places allow nav areas to be given a region name, AKA callouts.

It allows bots to communicate positions to human teammates.

# Database Files

NavBot's Navigation Mesh has three files to load place names from.

* A global file.
* A mod specific file.
* A map specific file.

# Creating Map Specific Place Database

Create a new text file at `configs/navbot/[mod]/maps/nav_places_[map].cfg`.

`[mod]` is the current mod gamefolder name.

`[map]` is the current map name report by the command `sm_nav_print_map_name`.

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

<!-- LINKS -->